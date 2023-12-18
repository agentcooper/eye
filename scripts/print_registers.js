console.log(`intptr_t v = 666;`);
for (let i = 0; i < 31; i++) {
  console.log(`__asm__ volatile("mov %0, x${i}" : "=r"(v));`);
  console.log(`printf("x${i} = %p\\n", (void *)v);`);
}