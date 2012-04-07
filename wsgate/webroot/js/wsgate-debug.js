// Provide a generic window.BlobBuilder
// See: https://developer.mozilla.org/en/DOM/BlobBuilder
function gBB() {
    if ('BlobBuilder' in window) {
        return;
    }
    if ('MozBlobBuilder' in window) {
        window.BlobBuilder = window.MozBlobBuilder;
    }
    if ('WebKitBlobBuilder' in window) {
        window.BlobBuilder = window.WebKitBlobBuilder;
    }
    if ('MSBlobBuilder' in window) {
        window.BlobBuilder = window.MSBlobBuilder;
    }
}
gBB();

