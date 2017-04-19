use std::fmt;

#[derive(Clone, Copy, Debug)]
pub enum Action {
	None,
	Wait { us: u32 },
	GPIO { on: u32, off: u32 },
}

impl fmt::Display for Action {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		match *self {
			Action::None => {
				write!(f, "Action::None")
			},
			Action::Wait { us } => {
				write!(f, "Action::Wait {{ us: {} }}", us)
			},
			Action::GPIO { on, off } => {
				write!(f, "Action::GPIO {{ on: {:X}, off: {:X} }}", on, off)
			}
		}
	}
}