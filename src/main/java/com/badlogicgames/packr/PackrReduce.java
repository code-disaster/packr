/*******************************************************************************
 * Copyright 2014 See AUTHORS file.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

package com.badlogicgames.packr;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.io.filefilter.TrueFileFilter;
import org.zeroturnaround.zip.ZipUtil;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;

/**
 * Functions to reduce package size for both classpath JARs, and the bundled JRE.
 */
class PackrReduce {

	static void minimizeJre(File output, PackrConfig config) throws IOException {
		if (config.minimizeJre == null) {
			return;
		}

		System.out.println("Minimizing JRE ...");

		if (config.verbose) {
			System.out.println("  # Unpacking rt.jar ...");
		}

		ZipUtil.unpack(new File(output, "jre/lib/rt.jar"), new File(output, "jre/lib/rt"));

		// todo: detect if OracleJDK or OpenJDK
		minimizeOpenJre(output, config);
	}

	private static void minimizeOpenJre(File output, PackrConfig config) throws IOException {
		if (config.verbose) {
			System.out.println("  # Processing OpenJDK runtime ...");
		}

		if (config.verbose) {
			System.out.println("  # Removing executables ...");
		}

		if (config.platform == PackrConfig.Platform.Windows32 || config.platform == PackrConfig.Platform.Windows64) {
			FileUtils.deleteDirectory(new File(output, "jre/bin/client"));
			File[] files = new File(output, "jre/bin").listFiles();
			if (files != null) {
				for (File file : files) {
					if (file.getName().endsWith(".exe")) {
						PackrFileUtils.delete(file);
					}
				}
			}
		} else {
			FileUtils.deleteDirectory(new File(output, "jre/bin"));
		}

		String[] minimizeList = readMinimizeProfile(config);
		if (minimizeList.length > 0) {
			if (config.verbose) {
				System.out.println("  # Removing files and directories in profile '" + config.minimizeJre + "' ...");
			}

			for (String minimize : minimizeList) {
				minimize = minimize.trim();
				File file = new File(output, minimize);
				try {
					if (file.isDirectory()) {
						FileUtils.deleteDirectory(new File(output, minimize));
					} else {
						PackrFileUtils.delete(file);
					}
				} catch (IOException e) {
					System.out.println("Failed to delete " + file.getPath() + ": " + e.getMessage());
				}
			}
		}

		if (new File(output, "jre/lib/rhino.jar").exists()) {
			if (config.verbose) {
				System.out.println("  # Removing rhino.jar ...");
			}
			PackrFileUtils.delete(new File(output, "jre/lib/rhino.jar"));
		}
	}

	private static String[] readMinimizeProfile(PackrConfig config) throws IOException {
		String[] lines;

		if (new File(config.minimizeJre).exists()) {
			lines = FileUtils.readFileToString(new File(config.minimizeJre)).split("\r?\n");
		} else {
			InputStream in = Packr.class.getResourceAsStream("/minimize/" + config.minimizeJre);
			if (in != null) {
				lines = IOUtils.toString(in).split("\r?\n");
				in.close();
			} else {
				lines = new String[0];
			}
		}

		return lines;
	}

	static void repackJarFiles(File output, PackrConfig config) throws IOException {
		repackJarFile(new File(output, "jre/lib/rt.jar"), new File(output, "jre/lib/rt"), config);
	}

	private static void repackJarFile(File jar, File fromDir, PackrConfig config) throws IOException {
		System.out.println("Reducing " + jar.getName() + " ...");

		if (fromDir.exists()) {
			if (!fromDir.isDirectory()) {
				throw new IOException("Expecting directory, but file found: " + fromDir.getPath());
			}
		} else {
			if (config.verbose) {
				System.out.println("  # Unpacking " + jar.getName() + " first ...");
			}
			ZipUtil.unpack(jar, fromDir);
		}

		long beforeLen = jar.length();
		PackrFileUtils.delete(jar);

		if (config.verbose) {
			System.out.println("  # (Re-)packing " + jar.getName() + " ...");
		}

		ZipUtil.pack(fromDir, jar);
		FileUtils.deleteDirectory(fromDir);

		long afterLen = jar.length();

		if (config.verbose) {
			System.out.println("  # " + beforeLen / 1024 + " kb -> " + afterLen / 1024 + " kb");
		}
	}

	static void removePlatformLibs(File output, PackrConfig config) throws IOException {
		System.out.println("Removing foreign platform libs ...");

		// let's remove any shared libs not used on the platform, e.g. libGDX/LWJGL natives
		for (String classpath : config.classpath) {
			if (config.verbose) {
				System.out.println("  # Unpacking '" + classpath + "'");
			}

			File jar = new File(output, new File(classpath).getName());
			File jarDir = new File(output, jar.getName() + ".tmp");

			ZipUtil.unpack(jar, jarDir);

			Set<String> extensions = new HashSet<String>();

			switch (config.platform) {
				case Windows32:
				case Windows64:
					extensions.add(".dylib");
					extensions.add(".so");
					break;
				case Linux32:
				case Linux64:
					extensions.add(".dylib");
					extensions.add(".dll");
					break;
				case MacOS:
					extensions.add(".dll");
					extensions.add(".so");
					break;
			}

			for (Object obj : FileUtils.listFiles(jarDir, TrueFileFilter.INSTANCE , TrueFileFilter.INSTANCE)) {
				File file = new File(obj.toString());
				for (String extension: extensions) {
					if (file.getName().endsWith(extension)) {
						if (config.verbose) {
							System.out.println("  # Removing '" + file.getPath() + "'");
						}
						PackrFileUtils.delete(file);
					}
				}
			}

			if (config.verbose) {
				System.out.println("  # Repacking '" + classpath + "'");
			}

			PackrFileUtils.delete(jar);
			ZipUtil.pack(jarDir, jar);
			FileUtils.deleteDirectory(jarDir);
		}
	}

}
