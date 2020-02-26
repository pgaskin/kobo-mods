package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
)

// FindPIDByName finds the first PID with a cmdline[0] exactly matching the
// specified name.
func FindPIDByName(name string) (int, error) {
	nameb := []byte(name)
	fis, err := ioutil.ReadDir("/proc")
	if err != nil {
		return 0, err
	}
	for _, fi := range fis {
		pid, err := strconv.Atoi(fi.Name())
		if err != nil {
			continue
		}
		buf, err := ioutil.ReadFile("/proc/" + fi.Name() + "/cmdline")
		if err != nil {
			return 0, err
		}
		if i := bytes.IndexByte(buf, 0); i != -1 {
			buf = buf[:i]
		}
		if bytes.Equal(buf, nameb) {
			return pid, nil
		}
	}
	return 0, fmt.Errorf("no such process %#v", name)
}

// FindPIDMap finds the offset of the first map for pid with the specified name,
// which must not contain whitespace. Only maps with a file offset of 0 are
// currently supported.
func FindPIDMap(pid int, name string) (int64, error) {
	nameb := []byte(name)
	f, err := os.Open("/proc/" + strconv.Itoa(pid) + "/maps")
	if err != nil {
		return 0, err
	}
	defer f.Close()
	s := bufio.NewScanner(f)
	for s.Scan() {
		f := bytes.Fields(s.Bytes())
		if len(f) < 6 {
			continue
		}
		if !bytes.Equal(nameb, f[5]) {
			continue
		}
		off, err := strconv.ParseInt(string(bytes.Split(f[0], []byte{'-'})[0]), 16, 64)
		if err != nil {
			return 0, err
		}
		foff, err := strconv.ParseInt(string(f[2]), 16, 64)
		if err != nil {
			return 0, err
		} else if foff != 0 {
			return 0, fmt.Errorf("file offset %d not 0 (not handled here): %s", foff, s.Text())
		}
		return off, nil
	}
	if err := s.Err(); err != nil {
		return 0, err
	}
	return 0, fmt.Errorf("could not find file in maps")
}
