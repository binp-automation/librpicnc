use std::sync::atomic::{AtomicBool, Ordering};

use libc::{c_int};

mod error;
pub use self::error::Error;

// use generated::pigpio;

extern "C" {
	fn gpioInitialise() -> c_int;
	fn gpioTerminate();
}


lazy_static! {
	static ref INIT: AtomicBool = AtomicBool::new(false);
}


pub struct GPIO {

}

impl GPIO {
	pub fn new() -> Result<GPIO, Error> {
		if !INIT.compare_and_swap(false, true, Ordering::SeqCst) {
			unsafe { gpioInitialise(); }
			Ok(GPIO { })
		} else {
			Err(Error::new())
		}
	}
}

impl Drop for GPIO {
	fn drop(&mut self) {
		if INIT.compare_and_swap(true, false, Ordering::SeqCst) {
			unsafe { gpioTerminate(); }
		} else {
			panic!("'initialized' flag isn't set");
		}
	}
}