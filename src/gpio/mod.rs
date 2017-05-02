use std::sync::atomic::{AtomicBool, Ordering};

use libc::{c_int, c_uint};

mod error;
pub use self::error::Error;

use generated::pigpio;

extern "C" {
	fn gpioInitialise() -> c_int;
	fn gpioTerminate();

	fn gpioSetMode(gpio: c_uint, mode: c_uint) -> c_int;
}


lazy_static! {
	static ref INIT: AtomicBool = AtomicBool::new(false);
}


pub struct GPIO {

}

impl GPIO {
	pub fn new() -> Result<GPIO, Error> {
		if !INIT.compare_and_swap(false, true, Ordering::SeqCst) {
			let r = unsafe { gpioInitialise() };
			if r < 0 {
				Err(Error::new(r))
			} else {
				Ok(GPIO { })
			}
		} else {
			Err(Error::new(0))
		}
	}
}

impl Drop for GPIO {
	fn drop(&mut self) {
		if INIT.compare_and_swap(true, false, Ordering::SeqCst) {
			unsafe { gpioTerminate(); }
		} else {
			panic!("'INIT' flag isn't set");
		}
	}
}

pub enum Mode {
	Input,
	Output,
	Alt(i32),
}

impl GPIO {
	pub fn set_mode(&mut self, pin: u32, mode: Mode) -> Result<(), Error> {
		let m = match mode {
			Mode::Input => pigpio::PI_INPUT,
			Mode::Output => pigpio::PI_OUTPUT,
			Mode::Alt(i) => {
				match i {
					0 => pigpio::PI_ALT0,
					1 => pigpio::PI_ALT1,
					2 => pigpio::PI_ALT2,
					3 => pigpio::PI_ALT3,
					4 => pigpio::PI_ALT4,
					5 => pigpio::PI_ALT5,
					_ => 0xFF,
				}
			}
		} as c_uint;
		let r = unsafe { gpioSetMode(pin as c_uint, m) };
		if r == 0 {
			Ok(())
		} else {
			Err(Error::new(r))
		}
	}
}