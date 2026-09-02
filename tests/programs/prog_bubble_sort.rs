use std::io::{self, BufRead};

fn bubble_sort(arr: &mut [i32]) {
    let n = arr.len();
    for i in 0..n - 1 {
        for j in 0..n - i - 1 {
            if arr[j] > arr[j + 1] {
                arr.swap(j, j + 1);
            }
        }
    }
}

fn main() {
    let stdin = io::stdin();
    let mut lines = stdin.lock().lines();
    let n: usize = lines.next().unwrap().unwrap().trim().parse().unwrap();
    let mut arr: Vec<i32> = Vec::with_capacity(n);
    for _ in 0..n {
        arr.push(lines.next().unwrap().unwrap().trim().parse().unwrap());
    }
    bubble_sort(&mut arr);
    for x in arr {
        println!("{}", x);
    }
}
