default:
    just --list

build *args:
    uv run build/build.py {{args}}

build-release *args:
    uv run build/build.py -r {{args}}

run proj *args: (build proj)
    _bin/{{proj}}-debug {{args}}

run-release proj *args: (build-release proj)
    _bin/{{proj}} {{args}}

clean:
    rm -rf _bin _obj

alias b := build
alias br := build-release
alias r := run
alias rr := run-release
alias c := clean
