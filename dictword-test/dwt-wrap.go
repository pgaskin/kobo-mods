// Command dwt-wrap wraps dictword-test.so to concurrently generate prefixes for
// words listed in the arguments or stdin.
package main

import (
	"bufio"
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"runtime"
	"strings"
	"sync"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s (-|words...)\n", os.Args[0])
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
						och <- fmt.Sprintf(`// %#v: err(%v),`, word, err)
					} else {
						och <- fmt.Sprintf(`%#v: %#v,`, word, prefix)
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

func getPrefix(word string) (string, error) {
	cmd := exec.Command("./dictword-test.so", word)
	cmd.Env = append(cmd.Env, "LD_PRELOAD=./dictword-test.so")

	stdout := bytes.NewBuffer(nil)
	cmd.Stdout = stdout

	stderr := bytes.NewBuffer(nil)
	cmd.Stderr = stderr

	if err := cmd.Run(); err != nil {
		return "", fmt.Errorf("dictword-test.so: %v (stderr: %#v)", err, stderr.String())
	}

	if dictpath := strings.TrimSpace(stdout.String()); len(dictpath) == 0 {
		return "", fmt.Errorf("dictword-test.so: no output (stderr: %#v)", stderr.String())
	} else if dicthtml := strings.TrimPrefix(dictpath, "/mnt/onboard/.kobo/dict/dicthtml.zip/"); dictpath == dicthtml {
		return "", fmt.Errorf("dictword-test.so: path does not start with expected prefix (path: %#v)", dictpath)
	} else if prefix := strings.TrimSuffix(dicthtml, ".html"); dicthtml == prefix {
		return "", fmt.Errorf("dictword-test.so: path does end start with expected suffix (path: %#v)", dictpath)
	} else {
		return prefix, nil
	}
}
