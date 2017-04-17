extern crate libc;

mod gen;

use gen::{Action, Generator as Gen};

fn main() {
	let mut gen = Gen::new().unwrap();
	gen.run(&mut || -> Action {
		println!("action: None");
		Action::None
	});
}
