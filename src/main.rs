#![allow(dead_code)]

extern crate libc;
#[macro_use]
extern crate lazy_static;

mod gpio;
mod gen;
mod generated;

use gpio::GPIO;
use gen::{Action, Generator as Gen};

fn main() {
	let mut gpio = GPIO::new().unwrap();
	let mut gen = Gen::options().loop_delay(100000).build().unwrap();
	let mut cnt = 1000;
	let pin = 18;
	// gpio.set_mode(pin, gpio::Mode::Output); // and remove from generator.c: gen_run
	gen.run(&mut gpio, &mut || -> Action {
		let action = if cnt > 0 {
			match cnt % 4 {
				0 => Action::GPIO { on: 1<<pin, off: 0 },
				2 => Action::GPIO { on: 0, off: 1<<pin },
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
