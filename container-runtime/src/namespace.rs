use crate::container::{ContainerConfig, ContainerError};

/// Create namespaces and execute the container process (Linux only)
#[cfg(target_os = "linux")]
pub fn create_and_exec(
    config: &ContainerConfig,
    rootfs: &str,
    container_id: &str,
) -> Result<(), ContainerError> {
    use nix::sched::{clone, CloneFlags};
    use nix::sys::signal::Signal;
    use nix::sys::wait::waitpid;

    let clone_flags = CloneFlags::CLONE_NEWPID
        | CloneFlags::CLONE_NEWNS
        | CloneFlags::CLONE_NEWUTS
        | CloneFlags::CLONE_NEWNET;

    let rootfs = rootfs.to_string();
    let hostname = config.hostname.clone();
    let command = config.command.clone();
    let cid = container_id.to_string();

    // Stack for the cloned process
    let mut stack = vec![0u8; 1024 * 1024]; // 1MB stack

    let cb = Box::new(move || -> isize {
        if let Err(e) = child_process(&rootfs, &hostname, &command, &cid) {
            eprintln!("Child error: {}", e);
            return 1;
        }
        0
    });

    let pid = clone(cb, &mut stack, clone_flags, Some(Signal::SIGCHLD as i32))
        .map_err(|e| ContainerError::Namespace(format!("clone failed: {}", e)))?;

    // Parent waits for child
    waitpid(pid, None)
        .map_err(|e| ContainerError::Namespace(format!("waitpid failed: {}", e)))?;

    Ok(())
}

#[cfg(target_os = "linux")]
fn child_process(
    rootfs: &str,
    hostname: &str,
    command: &[String],
    container_id: &str,
) -> Result<(), ContainerError> {
    use nix::unistd::sethostname;
    use std::ffi::CString;

    // Set hostname
    sethostname(hostname)
        .map_err(|e| ContainerError::Namespace(format!("sethostname failed: {}", e)))?;

    // Set up filesystem isolation
    crate::filesystem::setup_rootfs(rootfs)
        .map_err(|e| ContainerError::Filesystem(e.to_string()))?;

    // Add process to cgroup
    crate::cgroup::add_process(container_id, std::process::id())
        .map_err(|e| ContainerError::Cgroup(e.to_string()))?;

    // Exec the command
    let program = CString::new(command[0].as_str())
        .map_err(|e| ContainerError::Exec(e.to_string()))?;
    let args: Vec<CString> = command
        .iter()
        .map(|a| CString::new(a.as_str()).unwrap())
        .collect();

    nix::unistd::execvp(&program, &args)
        .map_err(|e| ContainerError::Exec(format!("execvp failed: {}", e)))?;

    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn create_and_exec(
    _config: &ContainerConfig,
    _rootfs: &str,
    _container_id: &str,
) -> Result<(), ContainerError> {
    Err(ContainerError::Namespace(
        "Namespaces require Linux".to_string(),
    ))
}
