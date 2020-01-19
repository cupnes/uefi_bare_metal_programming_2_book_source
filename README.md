# 『フルスクラッチで作る!UEFIベアメタルプログラミング パート2』 SOURCE FILES

This is a [Re:VIEW](https://reviewml.org/) format source files for my book 『フルスクラッチで作る!UEFIベアメタルプログラミング パート2』.

PDF and HTML versions of this book are available at http://yuma.ohgami.jp .

## Setup

Please use the following docker container.

 * [yohgami/review - Docker Hub](https://hub.docker.com/r/yohgami/review)

### Docker Pull Command

```sh
$ docker pull yohgami/review
```

## How to Generate a PDF File or HTML Files

### PDF

```sh
$ ./build-in-docker.sh pdf
```

Generated as follows:

```sh
$ ls articles/*.pdf
articles/UEFI-Bare-Metal-Programming-2.pdf
```

### HTML

```sh
$ ./build-in-docker.sh html
```

Generated as follows:

```sh
$ ls articles/html
01_simple_text_output.html    03_load_start_image.html  05_other_bs_rs.html  index.html  postscript.html  style.css
02_simple_text_input_ex.html  04_evt_timer.html         images               intro.html  refs.html
```

## Reference

The Re:VIEW style files and the Docker container used the following repository content:

 * [C89-FirstStepReVIEW-v2](https://github.com/TechBooster/C89-FirstStepReVIEW-v2)
