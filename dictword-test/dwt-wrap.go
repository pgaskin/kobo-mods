// Command dwt-wrap wraps dictword-test.so to concurrently generate prefixes for
// words listed in the arguments or stdin.
package main

import (
	"bufio"
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s (- | words...)\n", os.Args[0])
		os.Exit(0)
		return
	}

	var twg sync.WaitGroup

	// Input words
	ich := make(chan string)

	twg.Add(1)
	go func() {
		defer twg.Done()
		defer close(ich)

		var iwg sync.WaitGroup
		if len(os.Args) == 2 && os.Args[1] == "-" {
			iwg.Add(1)
			go func() {
				defer iwg.Done()

				sc := bufio.NewScanner(os.Stdin)
				for sc.Scan() {
					if len(sc.Bytes()) > 0 {
						ich <- sc.Text()
					}
				}
				if err := sc.Err(); err != nil {
					panic(err)
				}
			}()
		} else {
			iwg.Add(1)
			go func() {
				defer iwg.Done()

				for _, word := range os.Args[1:] {
					ich <- word
				}
			}()
		}

		iwg.Wait()
	}()

	// Output prefixes
	och := make(chan string)

	twg.Add(1)
	go func() {
		defer twg.Done()
		defer close(och)

		var owg sync.WaitGroup
		for i := 0; i < runtime.NumCPU()*4; i++ {
			owg.Add(1)
			go func() {
				defer owg.Done()

				for word := range ich {
					prefix, err := getPrefix(word)
					if err != nil {
						och <- fmt.Sprintf(`// {%#v, err(%v)},`, word, err)
					} else {
						och <- fmt.Sprintf(`{%#v, %#v},`, word, prefix)
					}
				}
			}()
		}

		owg.Wait()
	}()

	// Write
	twg.Add(1)
	go func() {
		defer twg.Done()
		for line := range och {
			fmt.Println(line)
		}
	}()

	// Wait for pipeline to finish
	twg.Wait()
}

const soname = "dictword-test.so"

func getPrefix(word string) (string, error) {
	dwt := dwtso()
	cmd := exec.Command(dwt, word)
	cmd.Env = append(cmd.Env, "LD_PRELOAD="+dwt)

	stdout := bytes.NewBuffer(nil)
	cmd.Stdout = stdout

	stderr := bytes.NewBuffer(nil)
	cmd.Stderr = stderr

	if err := cmd.Run(); err != nil {
		return "", fmt.Errorf("%s: %v (stderr: %#v)", soname, err, stderr.String())
	}

	if dictpath := strings.TrimSpace(stdout.String()); len(dictpath) == 0 {
		return "", fmt.Errorf("%s: no output (stderr: %#v)", soname, stderr.String())
	} else if dicthtml := strings.TrimPrefix(dictpath, "/mnt/onboard/.kobo/dict/dicthtml.zip/"); dictpath == dicthtml {
		return "", fmt.Errorf("%s: path does not start with expected prefix (path: %#v)", soname, dictpath)
	} else if prefix := strings.TrimSuffix(dicthtml, ".html"); dicthtml == prefix {
		return "", fmt.Errorf("%s: path does end start with expected suffix (path: %#v)", soname, dictpath)
	} else {
		return prefix, nil
	}
}

func dwtso() string {
	p := "." + string(os.PathSeparator) + soname // we don't want filepath.Join trimming the .
	if _, err := os.Stat(p); err == nil {
		return p
	}
	if exc, err := os.Executable(); err == nil {
		p = filepath.Join(filepath.Dir(exc), soname)
		if _, err := os.Stat(p); err == nil {
			return p
		}
	}
	if pt, err := exec.LookPath(soname); err == nil {
		return pt
	}
	return p
}
