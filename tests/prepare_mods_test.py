import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("prepare_mods", Path(__file__).resolve().parents[1] / "tools/prepare_mods.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class LaunchFolderTests(unittest.TestCase):
    def test_only_integrated_standalones_are_excluded(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            source.mkdir()
            retained = {"com.blueflaregames.zora_swim.dusk", "vbao.dusk"}
            originals = retained | module.INTEGRATED
            for name in originals:
                (source / name).write_bytes(b"original")
            overhaul = root / "new-build.dusk"
            overhaul.write_bytes(b"new build")
            staged = module.prepare(source, root / "staged", overhaul)
            self.assertEqual({p.name for p in staged.glob("*.dusk")}, retained | {"tp_remaster.dusk"})
            self.assertEqual((staged / "tp_remaster.dusk").read_bytes(), b"new build")
            self.assertEqual({p.name for p in source.iterdir()}, originals)
            (source / "vbao.dusk").unlink()
            module.prepare(source, staged, overhaul)
            self.assertFalse((staged / "vbao.dusk").exists())

    def test_unmanaged_files_are_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            source.mkdir()
            overhaul = root / "new-build.dusk"
            overhaul.touch()
            staged = root / "staged"
            staged.mkdir()
            user_file = staged / "tp_remaster.dusk"
            user_file.write_bytes(b"user file")
            with self.assertRaises(ValueError):
                module.prepare(source, staged, overhaul)
            self.assertEqual(user_file.read_bytes(), b"user file")


if __name__ == "__main__":
    unittest.main()
