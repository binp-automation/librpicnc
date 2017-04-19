extern crate gcc;
extern crate cmacros;

use std::fs::File;
use std::io::{Read, Write};

fn main() {
	let mut header_file = File::open("src/gen/c/generator.h").unwrap();
	let mut header_src = String::new();
	header_file.read_to_string(&mut header_src).unwrap();
	let macros = cmacros::extract_macros(&header_src).unwrap();
	let rust_src = cmacros::generate_rust_src(&macros, |def| cmacros::translate_macro(def));
	let mut output_file = File::create("src/generated/generator.rs").unwrap();
	output_file.write(rust_src.as_bytes()).unwrap();

	gcc::Config::new()
		.file("src/gen/c/generator.c")
		.include("src/gen/c/")
		.include("pigpio/")
		.compile("libgenerator.a");
	println!("cargo:rustc-link-lib=generator");


	println!("cargo:rustc-link-search=pigpio");
	println!("cargo:rustc-link-lib=pigpio");

}