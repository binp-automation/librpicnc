mod axis;
pub use self::axis::Axis;

use gen::{Action, Generator};

pub trait Device {
	fn next_action(&mut self) -> Action;
	fn run(&mut self, gen: &mut Generator) {
		gen.run(&mut || { self.next_action() });
	}
}