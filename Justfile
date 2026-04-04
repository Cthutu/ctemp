default:
    just --list

python-env:
    uv --directory build sync

build *args: python-env
    build/.venv/bin/python build/build.py {{args}}

build-release *args: python-env
    build/.venv/bin/python build/build.py -r {{args}}

test *args: python-env
    build/.venv/bin/python build/test.py {{args}}

test-release *args: python-env
    build/.venv/bin/python build/test.py -r {{args}}

run proj *args: (build proj)
    _bin/{{proj}}-debug {{args}}

run-release proj *args: (build-release proj)
    _bin/{{proj}} {{args}}

clean:
    rm -rf _bin _obj
    rm -rf build/.venv
    rm -rf build/__pycache__

alias b := build
alias br := build-release
alias t := test
alias tr := test-release
alias r := run
alias rr := run-release
alias c := clean
