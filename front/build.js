const fs = require("fs").promises;
const path = require("path");
const esbuild = require("esbuild");
const { solidPlugin } = require("esbuild-plugin-solid");

const iconsDir = path.resolve("src/icons");
const iconsFile = path.resolve("src/icons.jsx");
const indexFile = path.resolve("src/index.jsx");
const distDir = path.resolve("dist");

async function parseIcon(file) {
	try {
		const content = await fs.readFile(path.join(iconsDir, file), "utf8");
		const name = path.basename(file, ".svg");
		const compName =
			name
			.split("_")
			.map((part) => part.charAt(0).toUpperCase() + part.slice(1))
			.join("") + "Svg";
		return `export const ${compName} = () => (${content.match(/<svg.*<\/svg>/s)[0]});`;
	} catch (err) {
		throw new Error(`\t❌ Error parsing ${file}: ${err.message}`);
	}
}

async function genIcons() {
	const files = (await fs.readdir(iconsDir)).filter((f) => f.endsWith(".svg"));
	const icons = await Promise.all(files.map(parseIcon));
	const code = icons.join("\n\n");
	await fs.writeFile(iconsFile, code, "utf8");
}

async function configBuild(target) {
	return esbuild.context({
		entryPoints: [indexFile],
		bundle: true,
		outdir: distDir,
		format: "esm",
		target: "esnext",
		sourcemap: target === "debug" ? "inline" : false,
		minify: target === "release",
		plugins: [solidPlugin()],
	});
}

async function cleanTmp() {
	await fs.rm(iconsFile, { force: true });
}

async function copyHtmlCss() {
	const files = await fs.readdir("src");
	for(const file of files) {
		if(file.endsWith(".html") || file.endsWith(".css")) {
			await fs.copyFile(path.join("src", file), path.join(distDir, file));
			console.log(`Copied ${file}`);
		}
	}
}

async function handleServer() {
	return new Promise((resolve) => {
		console.log("To end the server, type 'exit'");
		process.stdin.resume();
		process.stdin.on("data", (comm) => {
			if(comm.toString().trim() === "exit") {
				process.stdin.pause();
				resolve();
			}
		});
	});
}


async function build(target) {
	try {
		console.log(`Building for target: ${target}`);
		await copyHtmlCss();
		console.log("Generating icons...");
		await genIcons();
		console.log("✅ Icons generated");
		console.log("Building bundle...");
		const ctx = await configBuild(target);

		if(target === 'release') {
			await ctx.rebuild();
			console.log("✅ Built!");
		}

		if(target === 'debug') {
			const server = await ctx.serve({
				host: 'webvplayer',
				port: 3000,
				servedir: distDir
			});
			console.log("✅ Built!");
			await handleServer();
		}

		await ctx.dispose();

	} catch (err) {
		console.error(err.message);
	} finally {
		await cleanTmp();
	}
}

build(process.argv[2] || "release");

