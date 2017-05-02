extern crate gcc;
extern crate cmacros;

use std::fs::{File, OpenOptions};
use std::io::{Read, Write};

fn generate(src: &str, dst: &str, blacklist: &[&str]) {
	let mut header_file = File::open(src).unwrap();
	let mut regenerate = false;
	let mut output_file = match OpenOptions::new().write(true).open(dst) {
		Ok(file) => file,
		Err(..) => { regenerate = true; File::create(dst).unwrap() },
	};

	let src_time = header_file.metadata().unwrap().modified().unwrap();
	let dst_time = output_file.metadata().unwrap().modified().unwrap();
	if regenerate || src_time > dst_time {
		let mut header_src = String::new();
		header_file.read_to_string(&mut header_src).unwrap();
		let macros = cmacros::extract_macros(&header_src).unwrap();
		let rust_src = cmacros::generate_rust_src(&macros, |ref def| {
			let mut skip = false;
			for s in blacklist {
				if def.name.starts_with(s) {
					skip = true;
				}
			}
			if skip {
				cmacros::TranslateAction::Skip
			} else {
				cmacros::translate_macro(def)
			}
			
		});
		output_file.write(rust_src.as_bytes()).unwrap();
		println!("[ info ] generate '{}' file from '{}' header", dst, src);
	}
	println!("cargo:rerun-if-changed={}", src);
	println!("cargo:rerun-if-changed={}", dst);
}

fn compile(srcs: &[&str], inc_dirs: &[&str], incs: &[&str], lib: &str) {
	let mut cfg = gcc::Config::new();
	for src in srcs {
		cfg.file(src);
		println!("cargo:rerun-if-changed={}", src);
	}
	for inc_dir in inc_dirs {
		cfg.include(inc_dir);
	}
	for inc in incs {
		println!("cargo:rerun-if-changed={}", inc);
	}
	cfg.compile(&(String::from("lib") + lib + ".a"));
	println!("[ info ] compile '{}' library", lib);
	println!("cargo:rustc-link-lib={}", lib);
}

fn main() {
	generate(
		"src/gen/c/generator.h",
		"src/generated/generator.rs",
		&[]
		);
	compile(
		&["src/gen/c/generator.c"],
		&["src/gen/c/", "pigpio/"],
		&["src/gen/c/ringbuffer.h", "pigpio/pigpio.h"],
		"generator"
	);


	generate(
		"pigpio/pigpio.h",
		"src/generated/pigpio.rs",
		&["PI_DEFAULT"]
		);
	println!("cargo:rustc-link-search=pigpio");
	println!("cargo:rustc-link-lib=pigpio");

}