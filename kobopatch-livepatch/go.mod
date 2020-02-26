module github.com/geek1011/kobo-mods/kobopatch-livepatch

go 1.13

require (
	github.com/geek1011/kobopatch v0.14.2-0.20200226025105-71fa1456a9d6
	github.com/spf13/pflag v1.0.5
)

// from kobopatch/go.mod@0.14.1
replace gopkg.in/yaml.v3 => github.com/geek1011/yaml v0.0.0-20190717135119-db0123c0912e // v3-node-decodestrict
