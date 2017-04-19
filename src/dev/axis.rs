use device::Device;

use gen::{Generator as Gen};

#[derive(Clone, Copy, Debug)]
pub enum Dir {
	Left,
	Right,
}

pub struct Axis {
	pin_step: i32,
	pin_dir: i32,
	pin_left: i32,
	pin_right: i32,

	min_speed: f32,
	max_speed: f32,
	max_accel: f32,

	pos: i32,
	dir: Dir,
	length: u32,
}
