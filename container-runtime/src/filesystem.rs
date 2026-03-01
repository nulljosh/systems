/// Copy rootfs from source to destination
#[cfg(target_os = "linux")]
pub fn copy_rootfs(src: &str, dst: &str) -> Result<(), String> {
    use std::process::Command;

    let status = Command::new("cp")
        .args(["-a", src, dst])
        .status()
        .map_err(|e| format!("cp failed: {}", e))?;

    if !status.success() {
        return Err("cp returned non-zero".to_string());
    }
    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn copy_rootfs(_src: &str, _dst: &str) -> Result<(), String> {
    Ok(())
}

/// Set up filesystem isolation: pivot_root, mount proc/sys (Linux only)
#[cfg(target_os = "linux")]
pub fn setup_rootfs(rootfs: &str) -> Result<(), String> {
    use nix::mount::{mount, umount2, MntFlags, MsFlags};
    use nix::unistd::{chdir, pivot_root};
    use std::fs;
    use std::path::Path;

    let rootfs_path = Path::new(rootfs);

    // Make rootfs a mount point (bind mount to itself)
    mount(
        Some(rootfs),
        rootfs,
        None::<&str>,
        MsFlags::MS_BIND | MsFlags::MS_REC,
        None::<&str>,
    )
    .map_err(|e| format!("bind mount rootfs failed: {}", e))?;

    // Create required directories
    let proc_dir = rootfs_path.join("proc");
    let sys_dir = rootfs_path.join("sys");
    let dev_dir = rootfs_path.join("dev");
    let old_root = rootfs_path.join("oldroot");

    for dir in [&proc_dir, &sys_dir, &dev_dir, &old_root] {
        fs::create_dir_all(dir).map_err(|e| format!("mkdir failed: {}", e))?;
    }

    // Pivot root
    chdir(rootfs).map_err(|e| format!("chdir to rootfs failed: {}", e))?;
    pivot_root(".", "oldroot")
        .map_err(|e| format!("pivot_root failed: {}", e))?;
    chdir("/").map_err(|e| format!("chdir to / failed: {}", e))?;

    // Mount proc
    mount(
        Some("proc"),
        "/proc",
        Some("proc"),
        MsFlags::empty(),
        None::<&str>,
    )
    .map_err(|e| format!("mount proc failed: {}", e))?;

    // Mount sysfs
    mount(
        Some("sysfs"),
        "/sys",
        Some("sysfs"),
        MsFlags::empty(),
        None::<&str>,
    )
    .map_err(|e| format!("mount sysfs failed: {}", e))?;

    // Unmount old root
    umount2("/oldroot", MntFlags::MNT_DETACH)
        .map_err(|e| format!("umount oldroot failed: {}", e))?;
    fs::remove_dir("/oldroot").ok();

    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn setup_rootfs(_rootfs: &str) -> Result<(), String> {
    Err("Filesystem isolation requires Linux".to_string())
}
