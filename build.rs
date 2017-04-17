extern crate gcc;

fn main() {
	gcc::Config::new()
		.file("src/gen/generator.c")
		.include("src/gen")
		.include("pigpio")
		.compile("libgenerator.a");
	println!("cargo:rustc-link-lib=generator");

	println!("cargo:rustc-link-search=pigpio");
	println!("cargo:rustc-link-lib=pigpio");
}