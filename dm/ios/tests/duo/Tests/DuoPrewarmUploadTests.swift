import XCTest
@testable import DuoHarness

final class DuoPrewarmUploadTests: XCTestCase {
    func testUploadUnavailableEvenWithEndpoint() {
        XCTAssertEqual(Bundle.main.object(forInfoDictionaryKey: "DMPrewarmUploadURL") as? String,
                       "https://script.google.com/macros/s/test/exec")
        XCTAssertFalse(PrewarmUpload.available)
        PrewarmUpload.schedule()
    }

    func testSnapshotOnlyIncludesCompletePipelineRows() throws {
        let file = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: file) }
        let shader = "S\t0123456789abcdef_b\t0\n"
        let render = "R\t" + String(repeating: "0", count: 1024) + "\n"
        try Data((shader + render + "R\t12").utf8).write(to: file)
        XCTAssertEqual(PrewarmUpload.snapshot(file), Data((shader + render).utf8))

        try Data((shader + "Unexpected content\n").utf8).write(to: file)
        XCTAssertNil(PrewarmUpload.snapshot(file))
    }
}
