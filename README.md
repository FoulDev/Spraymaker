# Spraymaker

Spraymaker is a tool to create high quality sprays in Source engine games such as Team Fortress 2, Left 4 Dead 2, and so on. It supports animation, fading (mipmaps), and both at the same time, all with an easy to use click-and-drag user interface.


## Features

* Click-and-drag UI
* Animated sprays
* Fading sprays (mipmaps)
* Directly import GIFs, WebMs, etc.
* Automatic installation of sprays into games


## TODO

- [x] Build for x86_64 Linux
- [ ] Build for x86_64 Windows
- [ ] Docker container for x86_64 Linux builds
- [ ] Docker container for x86_64 Windows builds
- [ ] Limit frame count of imports
  - Spraymaker will happily import an entire movie until it runs out of memory.
- [ ] Progress throbber on import
- [ ] Threaded preview generation
- [ ] Threaded importing
- [ ] Automatic pixel art detection, scaling, and optimal image format selection
  - Downscale pixel art based on block size.
  - Find the best possible format for the downscaled image within filesize constraint.
- [ ] Show real previews live async generation
  - Show the image after autocropping, scaling, and conversion to target format.


## License

    Spraymaker/Spraymaker5000 is a video game texture creation tool.
    Copyright (C) 2024 A Foul Dev <a@foul.dev>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
