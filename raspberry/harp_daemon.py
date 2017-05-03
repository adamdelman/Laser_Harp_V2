#!/usr/bin/env python2
import logging

from laser_harp.controllers import NoteControllerManager


def main():
    logging.basicConfig(level=logging.DEBUG)
    manager = NoteControllerManager(
        soundfont_path='/home/pi/FluidR3.sf2'
    )
    manager.loop()


if __name__ == '__main__':
    main()
