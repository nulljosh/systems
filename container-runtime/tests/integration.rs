#[cfg(test)]
#[cfg(target_os = "linux")]
mod tests {
    use std::process::Command;

    #[test]
    fn test_cli_help() {
        let output = Command::new("cargo")
            .args(["run", "--", "--help"])
            .output()
            .expect("Failed to run binary");
        let stdout = String::from_utf8_lossy(&output.stdout);
        assert!(stdout.contains("mycontainer"));
    }

    #[test]
    fn test_cli_run_missing_cmd() {
        let output = Command::new("cargo")
            .args(["run", "--", "run", "nonexistent"])
            .output()
            .expect("Failed to run binary");
        assert!(!output.status.success());
    }
}

// Ensure tests compile but skip on non-Linux
#[cfg(test)]
#[cfg(not(target_os = "linux"))]
mod tests {
    #[test]
    fn test_skipped_on_macos() {
        eprintln!("Container runtime tests require Linux -- skipping");
    }
}
