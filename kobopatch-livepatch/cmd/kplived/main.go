package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"sync"
	"syscall"
	"time"

	"github.com/spf13/pflag"
)

// TODO: handle nickel restarts by putting the maps under another map based on
// nickel's PID.

var version = "dev"

var mu sync.Mutex
var files = map[string][]*PatchOp{}
var pid int

type PatchOp struct {
	Offset int64
	// will always be the same length
	Find    []byte
	Replace []byte
}

func main() {
	// fun fact: 7191 is python3: "".join([str(ord(c)%10) for c in "kobo"])
	addr := pflag.StringP("addr", "a", ":7181", "The address to listen on")
	nickel := pflag.StringP("nickel", "n", "/usr/local/Kobo/nickel", "The process to patch")
	help := pflag.BoolP("help", "h", false, "Show this help text")
	pflag.Parse()

	if *help || pflag.NArg() != 0 {
		fmt.Fprintf(os.Stderr, "Usage: %s [options]\n\nVersion:\n  kobopatch-livepatch %s\n\nOptions:\n%s\nThis MUST only be called once per instance of nickel.\n", os.Args[0], version, pflag.CommandLine.FlagUsages())
		if pflag.NArg() != 0 {
			os.Exit(2)
		} else {
			os.Exit(0)
		}
		return
	}

	if npid, err := FindPIDByName(*nickel); err != nil {
		fmt.Fprintf(os.Stderr, "Error: could not get pid of nickel (%#v): %v.\n", *nickel, err)
		os.Exit(1)
		return
	} else {
		pid = npid
	}

	fmt.Printf("Listening on http://%s.\n", *addr)
	if err := http.ListenAndServe(*addr, http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/version" && r.Method == http.MethodGet {
			w.Write([]byte(version))
			return
		}

		filename := filepath.Clean(filepath.FromSlash(r.URL.Path))
		switch r.Method {
		case http.MethodGet:
			info(filename).WriteTo(w)
			return

		case http.MethodPut:
			dec := json.NewDecoder(r.Body)
			dec.DisallowUnknownFields()

			var ops []*PatchOp
			if err := dec.Decode(&ops); err != nil {
				resp{
					Status: http.StatusBadRequest,
					Data:   err.Error(),
				}.WriteTo(w)
				return
			}

			for _, op := range ops {
				if len(op.Find) != len(op.Replace) {
					resp{
						Status: http.StatusBadRequest,
						Data:   fmt.Sprintf("op %#v: length mismatch between find and replace", op),
					}.WriteTo(w)
					return
				}
			}

			apply(filename, ops...).WriteTo(w)
			return

		default:
			resp{
				Status: http.StatusMethodNotAllowed,
				Data:   "invalid http method",
			}.WriteTo(w)
			return
		}
	})); err != nil {
		fmt.Fprintf(os.Stderr, "Error: serve http: %v.\n", err)
		os.Exit(1)
		return
	}
}

func info(filename string) resp {
	fi, err := os.Stat(filename)
	if err != nil {
		return resp{
			Status: http.StatusNotFound,
			Data:   fmt.Sprintf("no such file: %#v", filename),
		}
	}

	off, err := FindPIDMap(pid, filename)
	if err != nil {
		return resp{
			Status: http.StatusInternalServerError,
			Data:   fmt.Sprintf("find map of %#v in pid %d: %v", filename, pid, err),
		}
	}

	return resp{
		Status: http.StatusOK,
		Data: jmap{
			"filename": filename,
			"size":     fi.Size(),
			"pid":      pid,
			"offset":   off,
		},
	}
}

func apply(filename string, ops ...*PatchOp) resp {
	mu.Lock()
	defer mu.Unlock()

	fmt.Printf("--- Patching %s.\n", filename)

	off, err := FindPIDMap(pid, filename)
	if err != nil {
		return resp{
			Status: http.StatusInternalServerError,
			Data:   fmt.Sprintf("find map of %#v in pid %d: %v", filename, pid, err),
		}
	}

	t, err := Attach(pid)
	if err != nil {
		return resp{
			Status: http.StatusInternalServerError,
			Data:   fmt.Sprintf("attach to pid %d: %v", pid, err),
		}
	}
	defer t.Close()

	var op *PatchOp
	for len(files[filename]) != 0 {
		fmt.Printf("    Reverting %#v.\n", op)
		files[filename], op = files[filename][:len(files[filename])-1], files[filename][len(files[filename])-1]
		actual := make([]byte, len(op.Replace))
		if _, err := t.ReadAt(actual, off+op.Offset); err != nil {
			fmt.Printf("        Error: %v (REBOOTING).\n", err)
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("revert previous op %#v: read current memory: %v (REBOOTING TO PREVENT INCONSISTENCIES)", op, err),
				After:  reboot,
			}
		}
		if !bytes.Equal(actual, op.Replace) {
			fmt.Printf("        Error: state mismatch (REBOOTING).\n")
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("revert previous op %#v: mismatch with expected state: expected %X, got %X (REBOOTING TO PREVENT INCONSISTENCIES)", op, op.Replace, actual),
				After:  reboot,
			}
		}
		if _, err := t.WriteAt(op.Find, off+op.Offset); err != nil {
			fmt.Printf("        Error: %v (REBOOTING).\n", err)
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("revert previous op %#v: revert to old state: %v (REBOOTING TO PREVENT INCONSISTENCIES)", op, err),
				After:  reboot,
			}
		}
	}

	for _, op := range ops {
		fmt.Printf("    Checking %#v.\n", op)
		actual := make([]byte, len(op.Find))
		if _, err := t.ReadAt(actual, off+op.Offset); err != nil {
			fmt.Printf("        Error: %v (now unpatched).\n", err)
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("check op %#v: read current memory: %v (nickel is now in an unpatched state)", op, err),
			}
		}
		if !bytes.Equal(actual, op.Find) {
			fmt.Printf("        Error: state mismatch (now unpatched).\n")
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("check op %#v: mismatch with expected state: expected %X, got %X (nickel is now in an unpatched state)", op, op.Replace, actual),
			}
		}
	}

	for _, op := range ops {
		fmt.Printf("    Applying %#v.\n", op)
		if _, err := t.WriteAt(op.Replace, off+op.Offset); err != nil {
			fmt.Printf("        Error: %v (REBOOTING).\n", err)
			return resp{
				Status: http.StatusInternalServerError,
				Data:   fmt.Sprintf("apply op %#v: revert to old state: %v (REBOOTING TO PREVENT INCONSISTENCIES)", op, err),
				After:  reboot,
			}
		}
		files[filename] = append(files[filename], op)
	}

	return resp{
		Status: http.StatusOK,
		Data:   jmap{},
	}
}

type jmap map[string]interface{}

type resp struct {
	Status int // anything other than 200 is an error
	Data   interface{}
	After  func()
}

func (r resp) WriteTo(w http.ResponseWriter) {
	w.Header().Set("Cache-Control", "private, no-cache")
	w.Header().Set("Expires", "0")
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(r.Status)

	var stt string
	if r.Status == http.StatusOK {
		stt = "success"
	} else {
		stt = "error"
	}

	json.NewEncoder(w).Encode(jmap{
		"status": stt,
		"data":   r.Data,
	})

	if r.After != nil {
		time.Sleep(time.Millisecond * 500)
		r.After()
	}
}

func reboot() {
	fmt.Printf("*** Force rebooting.\n")
	syscall.Sync()
	syscall.Reboot(syscall.LINUX_REBOOT_CMD_RESTART)
}
