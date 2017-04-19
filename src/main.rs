#![allow(dead_code)]

extern crate libc;

mod gen;

use gen::{Action, Generator as Gen};

fn main() {
	let mut gen = Gen::prepare().loop_delay(100000).build().unwrap();
	let mut cnt = 1000;
	gen.run(&mut || -> Action {
		let action = if cnt > 0 {
			match cnt % 4 {
				0 => Action::GPIO { on: 1<<18, off: 0 },
				2 => Action::GPIO { on: 0, off: 1<<18 },
				1|3 => Action::Wait { us: 1000 },
				_ => Action::None,
			}
		} else {
			Action::None
		};
		cnt -= 1;
		println!("{}: {}", cnt, action);
		action
	});
}
