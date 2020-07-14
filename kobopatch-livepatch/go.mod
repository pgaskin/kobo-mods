module github.com/pgaskin/kobo-mods/kobopatch-livepatch

go 1.13

require (
	github.com/pgaskin/kobopatch v0.15.1
	github.com/spf13/pflag v1.0.5
)

// from kobopatch/go.mod@0.14.1
replace gopkg.in/yaml.v3 => github.com/pgaskin/yaml v0.0.0-20190717135119-db0123c0912e // v3-node-decodestrict
