function fib(n: i64): i64 {
    if (n == 0) {
        return 0;
    }

    if (n == 1) {
        return 1;
    }

    return fib(n - 1) + fib(n - 2);
}

function main(): i64 {
    print(fib(40));
    return 0;
}