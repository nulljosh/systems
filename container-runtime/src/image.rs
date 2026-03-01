use std::fs;
use std::path::Path;

/// Unpack a tarball (optionally gzipped) into a rootfs directory
pub fn unpack_tarball(tarball_path: &str, rootfs_dir: &str) -> Result<(), String> {
    let file =
        fs::File::open(tarball_path).map_err(|e| format!("Failed to open tarball: {}", e))?;

    fs::create_dir_all(rootfs_dir).map_err(|e| format!("Failed to create rootfs dir: {}", e))?;

    let rootfs = Path::new(rootfs_dir);

    // Try gzipped first, fall back to plain tar
    if tarball_path.ends_with(".tar.gz") || tarball_path.ends_with(".tgz") {
        let decoder = flate2::read::GzDecoder::new(file);
        let mut archive = tar::Archive::new(decoder);
        archive
            .unpack(rootfs)
            .map_err(|e| format!("Failed to unpack gzipped tarball: {}", e))?;
    } else {
        let mut archive = tar::Archive::new(file);
        archive
            .unpack(rootfs)
            .map_err(|e| format!("Failed to unpack tarball: {}", e))?;
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_unpack_tarball() {
        let dir = tempfile::tempdir().unwrap();
        let tarball = dir.path().join("test.tar");
        let rootfs = dir.path().join("rootfs");

        // Create a simple tar file
        let file = fs::File::create(&tarball).unwrap();
        let mut builder = tar::Builder::new(file);

        let data = b"hello world";
        let mut header = tar::Header::new_gnu();
        header.set_size(data.len() as u64);
        header.set_mode(0o644);
        header.set_cksum();
        builder
            .append_data(&mut header, "test.txt", &data[..])
            .unwrap();
        builder.finish().unwrap();

        unpack_tarball(tarball.to_str().unwrap(), rootfs.to_str().unwrap()).unwrap();
        assert!(rootfs.join("test.txt").exists());
    }
}
