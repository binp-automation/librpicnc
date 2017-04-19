use std::error::Error as ErrorTrait;
use std::fmt;

#[derive(Debug)]
pub struct Error {}

impl Error {
	pub fn new() -> Error {
		Error {}
	}
}

impl ErrorTrait for Error {
	fn description(&self) -> &str {
		"Generator error"
	}
}

impl fmt::Display for Error {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		write!(f, "{}", self.description())
	}
}