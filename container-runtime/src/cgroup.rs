const CGROUP_ROOT: &str = "/sys/fs/cgroup";

/// Set up cgroup v2 resource limits (Linux only)
#[cfg(target_os = "linux")]
pub fn setup(container_id: &str, memory_limit: u64, cpu_weight: u64) -> Result<(), String> {
    use std::fs;

    let cgroup_path = format!("{}/{}", CGROUP_ROOT, container_id);

    // Create cgroup directory
    fs::create_dir_all(&cgroup_path)
        .map_err(|e| format!("Failed to create cgroup dir: {}", e))?;

    // Set memory limit
    let mem_max = format!("{}/memory.max", cgroup_path);
    fs::write(&mem_max, memory_limit.to_string())
        .map_err(|e| format!("Failed to set memory limit: {}", e))?;

    // Set CPU weight
    let cpu = format!("{}/cpu.weight", cgroup_path);
    fs::write(&cpu, cpu_weight.to_string())
        .map_err(|e| format!("Failed to set CPU weight: {}", e))?;

    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn setup(_container_id: &str, _memory_limit: u64, _cpu_weight: u64) -> Result<(), String> {
    // No-op on non-Linux
    Ok(())
}

/// Add a process to the cgroup (Linux only)
#[cfg(target_os = "linux")]
pub fn add_process(container_id: &str, pid: u32) -> Result<(), String> {
    use std::fs;

    let procs = format!("{}/{}/cgroup.procs", CGROUP_ROOT, container_id);
    fs::write(&procs, pid.to_string())
        .map_err(|e| format!("Failed to add process to cgroup: {}", e))?;
    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn add_process(_container_id: &str, _pid: u32) -> Result<(), String> {
    Ok(())
}

/// Clean up cgroup (Linux only)
#[cfg(target_os = "linux")]
pub fn cleanup(container_id: &str) -> Result<(), String> {
    use std::fs;

    let cgroup_path = format!("{}/{}", CGROUP_ROOT, container_id);
    if std::path::Path::new(&cgroup_path).exists() {
        fs::remove_dir(&cgroup_path)
            .map_err(|e| format!("Failed to remove cgroup: {}", e))?;
    }
    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn cleanup(_container_id: &str) -> Result<(), String> {
    Ok(())
}
