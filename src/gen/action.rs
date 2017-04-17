
pub enum Action {
	None,
	Wait { us: u32 },
	GPIO { on: u32, off: u32 },
}