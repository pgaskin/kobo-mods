GO = go

ifdef DOCKER
DOCKER_IMAGE ?= golang:1.13-buster
override GO := $(DOCKER) run --env=GOOS --env=GOARCH --env=GOARM --env=CGO_ENABLED --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir=/src --rm --entrypoint=/usr/local/go/bin/go $(DOCKER_IMAGE)
endif

all: kplived kplive

kplive-cross:
	$(DOCKER) run --rm --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir="/src" docker.elastic.co/beats-dev/golang-crossbuild:1.13.8-main   --platforms "linux/amd64"  --build-cmd 'CGO_ENABLED=1 go build -ldflags "-s -w -X main.version=$(shell git describe --tags --always) -linkmode external -extldflags -static" -o kplive-linux-64bit ./cmd/kplive'
	$(DOCKER) run --rm --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir="/src" docker.elastic.co/beats-dev/golang-crossbuild:1.13.8-main   --platforms "windows/386"  --build-cmd 'CGO_ENABLED=1 go build -ldflags "-s -w -X main.version=$(shell git describe --tags --always) -linkmode external -extldflags -static" -o kplive-windows.exe ./cmd/kplive'
	$(DOCKER) run --rm --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir="/src" docker.elastic.co/beats-dev/golang-crossbuild:1.13.8-darwin --platforms "darwin/amd64" --build-cmd 'CGO_ENABLED=1 go build -ldflags "-s -w -X main.version=$(shell git describe --tags --always)"                                        -o kplive-darwin-64bit ./cmd/kplive'

clean:
	rm -f kplived kplive kplive-*

.PHONY: all clean kplive-cross

kplived: ./cmd/kplived/*.go
	GOOS=linux GOARCH=arm GOARM=6 CGO_ENABLED=0 go build -ldflags "-s -w -X main.version=$(shell git describe --always --tags)" -o kplived ./cmd/kplived

kplive: ./cmd/kplive/*.go
	CGO_ENABLED=1 go build -ldflags "-s -w -X main.version=$(shell git describe --always --tags) -linkmode external -extldflags -static" -o kplive ./cmd/kplive
