function bar(): i64 {
    return 666;
}

function foo(n: i64): i64 {
    return n + 1;
}

function main(): i64 {
    let f1 = bar;
    print(f1());

    let f2 = foo;
    print(f2(665));
    return 0;
}