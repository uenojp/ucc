assert () {
    expected="$1"
    input="$2"

    ./ucc "$input" | cc -xassembler - -o tmp && ./tmp
    actual="$?"
    rm -f tmp

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"

echo OK
