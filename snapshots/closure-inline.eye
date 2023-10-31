function apply(x: i64, f: (x: i64) => i64): i64 {
    return f(x);
}

function foo(k: i64): void {
    print(apply(5, (x: i64): i64 => {
        return x + k;
    }));

    print(apply(5, (x: i64): i64 => {
        return x - k;
    }));
}

function main(): i64 {
    foo(10);
    return 0;
}