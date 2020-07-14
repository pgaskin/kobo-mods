package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"
	"runtime/debug"
	"strings"

	"github.com/pgaskin/kobopatch/patchfile"
	_ "github.com/pgaskin/kobopatch/patchfile/kobopatch"
	"github.com/pgaskin/kobopatch/patchlib"
	"github.com/spf13/pflag"
)

// TODO: clean everything up, this is currently a big mess

var version = "dev"

func versionStr() string {
	str := "kobopatch-livepatch " + version
	if bi, ok := debug.ReadBuildInfo(); ok {
		for _, d := range bi.Deps {
			if d.Path == "github.com/pgaskin/kobopatch" {
				str += " (kobopatch " + d.Version + ")"
				break
			}
		}
	}
	return str
}

func main() {
	kplivedBin := pflag.StringP("kplived-binary", "b", "", "The full path to the binary being patched (default: /usr/local/Kobo/{basename-of-source-binary}")
	// TODO: firmware := pflag.StringP("firmware", "f", "The full path to the firmware file to extract binaries from (default: none)")
	help := pflag.BoolP("help", "h", false, "Show this help text")
	pflag.Parse()

	if *help || pflag.NArg() != 3 {
		fmt.Fprintf(os.Stderr, "Usage: %s [options] kplived_hostname[:port=7181] binary patch_file\n\nVersion:\n  %s\n\nOptions:\n%s", os.Args[0], versionStr(), pflag.CommandLine.FlagUsages())
		if pflag.NArg() != 0 {
			os.Exit(2)
		} else {
			os.Exit(0)
		}
		return
	}

	fmt.Println(versionStr())

	addr := pflag.Args()[0]
	if !strings.Contains(addr, ":") {
		addr += ":7181"
	}
	if !strings.Contains(addr, "://") {
		addr = "http://" + addr
	}

	fmt.Printf("Checking kplived version on %#v.\n", addr)
	if err := func() error {
		resp, err := http.Get(addr + "/version")
		if err != nil {
			return err
		}
		defer resp.Body.Close()

		buf, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return err
		}

		if sver := string(bytes.TrimSpace(buf)); sver != version {
			return fmt.Errorf("kplived version mismatch: wanted %s, got %s", version, sver)
		}

		return nil
	}(); err != nil {
		fmt.Fprintf(os.Stderr, "Error: check version of kplived at %#v: %v.\n", addr, err)
		os.Exit(1)
		return
	}

	if *kplivedBin == "" {
		*kplivedBin = "/usr/local/Kobo/" + filepath.Base(pflag.Args()[1])
	}

	fmt.Printf("Checking binary %#v on %#v.\n", *kplivedBin, addr)
	if err := func() error {
		var obj struct {
			Filename string
			Offset   int64
			Pid      int
			Size     int64
		}
		if err := kplived(http.MethodGet, addr, *kplivedBin, nil, &obj); err != nil {
			return err
		}
		if obj.Filename != *kplivedBin {
			return fmt.Errorf("unexpected response filename: expected %#v, got %#v", *kplivedBin, obj.Filename)
		}
		// TODO: ensure file size matches local binary
		fmt.Printf("Found %#v at offset %d (size %d) into pid %d.\n", obj.Filename, obj.Offset, obj.Size, obj.Pid)
		return nil
	}(); err != nil {
		fmt.Fprintf(os.Stderr, "Error: check binary %#v: %v.\n", *kplivedBin, err)
		os.Exit(1)
		return
	}

	fmt.Printf("Parsing patch file %#v.\n", pflag.Args()[2])
	ps, err := patchfile.ReadFromFile("kobopatch", pflag.Args()[2])
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: parse patch file %#v: %v\n", pflag.Args()[2], err)
		os.Exit(1)
		return
	}

	fmt.Printf("Reading local source binary from %#v.\n", pflag.Args()[1])
	buf, err := ioutil.ReadFile(pflag.Args()[1])
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: read binary %#v: %v\n", pflag.Args()[1], err)
		os.Exit(1)
		return
	}

	// TODO: share struct layout with kplived
	var ops []struct {
		Offset  int64
		Find    []byte
		Replace []byte
	}

	fmt.Printf("Generating diff.\n")
	pt := patchlib.NewPatcher(buf)
	pt.Hook(func(offset int32, find, replace []byte) error {
		fmt.Printf(": @0x%08x %X => %X\n", offset, find, replace)
		op := struct {
			Offset  int64
			Find    []byte
			Replace []byte
		}{int64(offset), find, replace}
		if len(op.Replace) < len(op.Find) {
			op.Find = op.Find[:len(op.Replace)] // this happens for zlib patches
		}
		ops = append(ops, op)
		return nil
	})
	if err := ps.ApplyTo(pt); err != nil {
		fmt.Fprintf(os.Stderr, "Error: generate diff: %v\n", err)
		os.Exit(1)
		return
	}

	fmt.Printf("Reverting previous patches for %#v and applying new diff (%d ops).\n", *kplivedBin, len(ops))
	var obj struct{}
	if err := kplived(http.MethodPut, addr, *kplivedBin, ops, obj); err != nil {
		fmt.Fprintf(os.Stderr, "Error: apply patches: %v\n", err)
		os.Exit(1)
		return
	}

	fmt.Printf("Successfully applied patches (previous ones have been reverted) (keep in mind that you may need to reload whichever view is being patched, and that patches which patch stuff read during startup won't take effect).\n")
	os.Exit(0)
	return
}

func kplived(method, addr, filename string, in interface{}, out interface{}) error {
	var br io.Reader
	if in != nil {
		if buf, err := json.Marshal(in); err != nil {
			return fmt.Errorf("create request: encode input: %w", err)
		} else {
			br = bytes.NewReader(buf)
		}
	}

	req, err := http.NewRequest(method, addr+filename, br)
	if err != nil {
		return fmt.Errorf("create request: %w", err)
	}

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return fmt.Errorf("do request: %w", err)
	}
	defer resp.Body.Close()

	var robj struct {
		Status string
		Data   json.RawMessage
	}
	if err := json.NewDecoder(resp.Body).Decode(&robj); err != nil {
		return fmt.Errorf("decode response: %v", err)
	}

	if robj.Status != "success" {
		var msg string
		if err := json.Unmarshal(robj.Data, &msg); err != nil {
			msg = string(robj.Data)
		}
		return fmt.Errorf("kplived responded with an error (status %s): %s", resp.Status, msg)
	}

	if err := json.Unmarshal(robj.Data, &out); err != nil {
		return fmt.Errorf("decode kplived response: %w", err)
	}

	return nil
}
