package main

import "syscall"

// Tracee is not thread-safe. In addition, runtime.LockOSThread() must be used,
// as ptrace syscalls must come from the same OS thread.
type Tracee struct {
	pid int
	tid int
}

func Attach(pid int) (*Tracee, error) {
	if err := syscall.PtraceAttach(pid); err != nil {
		return nil, err
	}
	return &Tracee{pid, syscall.Gettid()}, nil
}

// Close detaches from the tracee.
func (t *Tracee) Close() error {
	t.checkTid()
	return syscall.PtraceDetach(t.pid)
}

func (t *Tracee) ReadAt(p []byte, off int64) (n int, err error) {
	t.checkTid()
	// Go nicely handles making the word size portable for us. In addition, on
	// Linux, PEEKTEXT and PEEKDATA are equivalent, so we don't need to worry
	// about that either.
	return syscall.PtracePeekText(t.pid, uintptr(off), p)
}

func (t *Tracee) WriteAt(p []byte, off int64) (n int, err error) {
	t.checkTid()
	return syscall.PtracePokeText(t.pid, uintptr(off), p)
}

func (t *Tracee) checkTid() {
	if t.tid != syscall.Gettid() {
		panic("called ptrace tracee from wrong thread")
	}
}
