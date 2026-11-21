"""Exercise the shared screen-shape policy at its boundaries."""
import ctypes
import subprocess
import tempfile
import unittest
from pathlib import Path

HOST = Path(__file__).resolve().parents[2] / "host"


class ScreenShapeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        library = Path(cls.directory.name) / "screen_shape.dylib"
        source = '''
#include "screen_shape.h"
double match_shape(double width, double height) { return DmMatchScreenShape(width, height); }
double wide_shape(double width, double height) { return DmWideScreenShape(width, height); }
double choice_taller(const char *choice, double width, double height) { return DmResolveScreenShape(choice, width, height).taller; }
int choice_wide(const char *choice, double width, double height) { return DmResolveScreenShape(choice, width, height).widescreen; }
unsigned taller_height(double ratio) { return DmTallerScreenHeight(ratio); }
'''
        subprocess.run(["cc", "-dynamiclib", "-x", "c", "-I", str(HOST), "-o", str(library), "-"],
                       input=source, text=True, check=True)
        cls.library = ctypes.CDLL(str(library))
        for name in ("match_shape", "wide_shape"):
            function = getattr(cls.library, name)
            function.argtypes = (ctypes.c_double, ctypes.c_double)
            function.restype = ctypes.c_double
        for name in ("choice_taller", "choice_wide"):
            function = getattr(cls.library, name)
            function.argtypes = (ctypes.c_char_p, ctypes.c_double, ctypes.c_double)
            function.restype = ctypes.c_double if name == "choice_taller" else ctypes.c_int
        cls.library.taller_height.argtypes = (ctypes.c_double,)
        cls.library.taller_height.restype = ctypes.c_uint

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_match_screen_boundaries(self):
        for width, height, expected in ((1, 1, 1.3), (1.3, 1, 1.3), (4, 3, 4 / 3),
                                        (1.77, 1, 1.77), (16, 9, 16 / 9),
                                        (2, 1, 2), (19.5, 9, 19.5 / 9),
                                        (21, 9, 19.5 / 9), (9, 21, 19.5 / 9)):
            with self.subTest(width=width, height=height):
                self.assertAlmostEqual(self.library.match_shape(width, height), expected)

    def test_wide_screen_boundaries(self):
        for width, height, expected in ((4, 3, 0), (16, 9, 0), (2, 1, 2),
                                        (19.5, 9, 19.5 / 9), (21, 9, 19.5 / 9),
                                        (9, 21, 0)):
            with self.subTest(width=width, height=height):
                self.assertAlmostEqual(self.library.wide_shape(width, height), expected)

    def test_invalid_dimensions(self):
        for width, height in ((0, 0), (1, 0), (-1, 1), (float("inf"), 1), (1, float("nan"))):
            with self.subTest(width=width, height=height):
                self.assertEqual(self.library.match_shape(width, height), 0)
                self.assertEqual(self.library.wide_shape(width, height), 0)

    def test_choice_controls_widescreen(self):
        for choice, width, height, taller, wide in ((b"auto", 1, 1, 1.3, 0),
                                                    (b"auto", 4, 3, 4 / 3, 0),
                                                    (b"auto", 1.77, 1, 1.77, 0),
                                                    (b"auto", 16, 9, 0, 1),
                                                    (b"auto", 21, 9, 0, 1),
                                                    (b"auto", 0, 0, 0, 0),
                                                    (b"0", 21, 9, 0, 0),
                                                    (b"1.43", 21, 9, 0, 0)):
            with self.subTest(choice=choice, width=width, height=height):
                self.assertAlmostEqual(self.library.choice_taller(choice, width, height), taller)
                self.assertEqual(self.library.choice_wide(choice, width, height), wide)

    def test_taller_output_keeps_minimum_and_existing_shapes(self):
        for ratio, expected in ((1.3, 1476), (1.301, 1476), (4 / 3, 1440), (1.43, 1344)):
            with self.subTest(ratio=ratio):
                height = self.library.taller_height(ratio)
                self.assertEqual(height, expected)
                self.assertEqual(height * 2 % 3, 0)
                self.assertGreaterEqual(1920 / height, 1.3)


if __name__ == "__main__":
    unittest.main()