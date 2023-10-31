function cons(a: i64, b: i64): (f: (a: i64, b: i64) => i64) => i64 {
    let inner = (f: (a: i64, b: i64) => i64): i64 => {
        return f(a, b);
    };
    return inner;
}

function car(p: (f: (a: i64, b: i64) => i64) => i64): i64 {
    let first = (a: i64, b: i64): i64 => {
        return a;
    };
    return p(first);
}

function cdr(p: (f: (a: i64, b: i64) => i64) => i64): i64 {
    let second = (a: i64, b: i64): i64 => {
        return b;
    };
    return p(second);
}

function main(): i64 {
    let pair = cons(3, 4);
    print(car(pair));
    print(cdr(pair));
    return 0;
}