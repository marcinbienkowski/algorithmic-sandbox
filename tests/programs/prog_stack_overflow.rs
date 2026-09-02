use std::hint::black_box;

fn recursive(depth: u32) {
    let x = black_box(42);
    if depth > 0 {
        recursive(depth - 1);
    }
    println!("{x}");
}

fn main() {
    recursive(10_000_000);
}
