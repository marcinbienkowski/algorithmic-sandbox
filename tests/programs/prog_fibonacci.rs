fn fibonacci(n: u32) -> u64 {
    match n {
        0 | 1 => n as u64,
        _ => {
            let mut a = 0u64;
            let mut b = 1u64;
            for _ in 2..=n {
                let c = a + b;
                a = b;
                b = c;
            }
            b
        }
    }
}

fn main() {
    const N: u32 = 20;
    (0..=N)
        .map(fibonacci)
        .for_each(|fib| println!("{fib}"));
}
