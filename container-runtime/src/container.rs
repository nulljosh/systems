use std::fmt;

/// Container configuration
pub struct ContainerConfig {
    pub image_path: String,
    pub command: Vec<String>,
    pub memory_limit: u64,
    pub cpu_weight: u64,
    pub hostname: String,
}

/// Container error type
#[derive(Debug)]
pub enum ContainerError {
    Namespace(String),
    Cgroup(String),
    Filesystem(String),
    Image(String),
    Exec(String),
}

impl fmt::Display for ContainerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ContainerError::Namespace(e) => write!(f, "Namespace error: {}", e),
            ContainerError::Cgroup(e) => write!(f, "Cgroup error: {}", e),
            ContainerError::Filesystem(e) => write!(f, "Filesystem error: {}", e),
            ContainerError::Image(e) => write!(f, "Image error: {}", e),
            ContainerError::Exec(e) => write!(f, "Exec error: {}", e),
        }
    }
}

impl std::error::Error for ContainerError {}

/// Run a container with the given configuration (Linux only)
#[cfg(target_os = "linux")]
pub fn run(config: ContainerConfig) -> Result<(), ContainerError> {
    use crate::{cgroup, filesystem, image, namespace};
    use std::path::Path;

    let container_id = format!("container-{}", std::process::id());
    let rootfs = format!("/tmp/{}/rootfs", container_id);

    // 1. Prepare rootfs from image
    let image_path = Path::new(&config.image_path);
    if image_path.is_file() {
        image::unpack_tarball(&config.image_path, &rootfs)
            .map_err(|e| ContainerError::Image(e.to_string()))?;
    } else if image_path.is_dir() {
        // Use directory directly as rootfs
        std::fs::create_dir_all(&rootfs)
            .map_err(|e| ContainerError::Filesystem(e.to_string()))?;
        // Bind mount or copy
        filesystem::copy_rootfs(&config.image_path, &rootfs)
            .map_err(|e| ContainerError::Filesystem(e.to_string()))?;
    } else {
        return Err(ContainerError::Image(format!(
            "Image not found: {}",
            config.image_path
        )));
    }

    // 2. Set up cgroups
    cgroup::setup(&container_id, config.memory_limit, config.cpu_weight)
        .map_err(|e| ContainerError::Cgroup(e.to_string()))?;

    // 3. Create namespaces and exec
    let result = namespace::create_and_exec(&config, &rootfs, &container_id);

    // 4. Cleanup
    let _ = cgroup::cleanup(&container_id);
    let _ = std::fs::remove_dir_all(format!("/tmp/{}", container_id));

    result
}

#[cfg(not(target_os = "linux"))]
pub fn run(_config: ContainerConfig) -> Result<(), ContainerError> {
    Err(ContainerError::Namespace(
        "Container runtime requires Linux".to_string(),
    ))
}
