// go run program.go

package main

import (
	"fmt"
)

type Counter struct {
	inc   func(int) int
	reset func()
}

func make_counter(initial int) Counter {
	counter := initial

	inc := func(by int) int {
		counter += by
		return counter
	}

	reset := func() {
		counter = initial
	}

	return Counter{inc, reset}
}

func main() {
	counter := make_counter(0)
	fmt.Println(counter.inc(42)) // 42
	fmt.Println(counter.inc(42)) // 84
	counter.reset()
	fmt.Println(counter.inc(1)) // 1
}
