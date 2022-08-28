from conans import ConanFile, CMake
import os

class Itzcoatl(ConanFile):
	name = "itzcoatl"
	description="Aztec themed snake game."
	version = "1.0"
	license="GNU GPLv3"
	author = "Ian Murfin - github.com/ianmurfinxyz - ianmurfin@protonmail.com"
	url = "https://github.com/ianmurfinxyz/itzcoatl"
	topics=("Arcade Game", "2D Games", "Pixiretro", "Game")
	settings = "os", "compiler", "build_type", "arch"
	generators = "cmake"
	exports_sources = ["CMakeLists.txt", "src/*", "include/*"]
	options = {"shared": [True, False], "fPIC": [True, False]}
	default_options = {"shared": False, "fPIC": True}

	def config_options(self):
		if self.settings.os == "Windows":
			del self.options.fPIC

	def requirements(self):
		self.requires("pixiretro/0.9.0@ianmurfinxyz/stable")

	def build(self):
		cmake = CMake(self)
		cmake.configure()
		cmake.build()

	def export(self):
		self.copy("README.md")
		self.copy("assets/**/*", excludes="assets/**/*.xcf")

	def package(self):
		self.copy("bin/itzcoatl", dst="", keep_path=False)
		self.copy("assets/**/*")
