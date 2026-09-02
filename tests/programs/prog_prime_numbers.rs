use std::io::{self, BufRead};

const MAX_NUM: usize = 1000000;

fn sieve() -> Vec<bool> {
    let mut is_prime = vec![true; MAX_NUM + 1];
    is_prime[0] = false;
    is_prime[1] = false;
    for i in 2.. {
        if i * i > MAX_NUM { break; }
        if is_prime[i] {
            for j in (i * i..=MAX_NUM).step_by(i) {
                is_prime[j] = false;
            }
        }
    }
    is_prime
}

fn main() {
    let is_prime = sieve();
    let stdin = io::stdin();
    let mut lines = stdin.lock().lines();
    let n: usize = lines.next().unwrap().unwrap().trim().parse().unwrap();
    for _ in 0..n {
        let num: usize = lines.next().unwrap().unwrap().trim().parse().unwrap();
        println!("{}", if num <= MAX_NUM && is_prime[num] { "YES" } else { "NO" });
    }
}
