use std::env;
use std::fmt::Write as FmtWrite;
use std::io::{self, Write};
use std::os::unix::fs::PermissionsExt;
use std::process::{Command, Output};
use std::thread;
use std::time::Duration;

type AppResult<T> = Result<T, String>;

#[derive(Clone)]
struct ProcessInfo {
    pid: String,
    user: String,
    state: String,
    cpu: String,
    memory: String,
    command: String,
}

#[derive(Clone)]
struct ServiceInfo {
    unit: String,
    state: String,
    description: String,
}

#[derive(Clone)]
struct InterfaceInfo {
    name: String,
    rx_bytes: u64,
    tx_bytes: u64,
}

#[derive(Clone)]
struct FlatpakApp {
    id: String,
    name: String,
    permissions: String,
}

struct Snapshot {
    processes: Vec<ProcessInfo>,
    services: Vec<ServiceInfo>,
    interfaces: Vec<InterfaceInfo>,
    connections: Vec<String>,
    flatpak_available: bool,
    flatpak_apps: Vec<FlatpakApp>,
}

fn command_output(arguments: &[&str]) -> AppResult<Output> {
    let program = arguments.first().copied().unwrap_or_default();
    Command::new(program)
        .args(&arguments[1..])
        .output()
        .map_err(|error| {
            if error.kind() == io::ErrorKind::NotFound {
                format!("Commande indisponible : {program}")
            } else {
                format!("Impossible de lancer {program} : {error}")
            }
        })
}

fn run_command(arguments: &[&str]) -> AppResult<String> {
    let program = arguments.first().copied().unwrap_or_default();
    let output = command_output(arguments)?;
    if !output.status.success() {
        let detail = String::from_utf8_lossy(&output.stderr).trim().to_owned();
        let fallback = String::from_utf8_lossy(&output.stdout).trim().to_owned();
        return Err(if !detail.is_empty() {
            detail
        } else if !fallback.is_empty() {
            fallback
        } else {
            format!("Échec de la commande : {program}")
        });
    }
    String::from_utf8(output.stdout).map_err(|error| {
        format!("La sortie de {program} n’est pas du texte UTF-8 valide : {error}")
    })
}

fn find_executable(program: &str) -> bool {
    env::var_os("PATH").is_some_and(|paths| {
        env::split_paths(&paths).any(|directory| {
            let candidate = directory.join(program);
            candidate.metadata().is_ok_and(|metadata| {
                metadata.is_file() && metadata.permissions().mode() & 0o111 != 0
            })
        })
    })
}

fn get_processes() -> AppResult<Vec<ProcessInfo>> {
    let output = run_command(&[
        "ps",
        "-eo",
        "pid=,user=,stat=,pcpu=,pmem=,comm=",
        "--sort=-pcpu",
    ])?;
    Ok(output
        .lines()
        .filter_map(|line| {
            let fields: Vec<_> = line
                .trim_start()
                .splitn(6, char::is_whitespace)
                .filter(|s| !s.is_empty())
                .collect();
            (fields.len() == 6).then(|| ProcessInfo {
                pid: fields[0].to_owned(),
                user: fields[1].to_owned(),
                state: fields[2].to_owned(),
                cpu: fields[3].to_owned(),
                memory: fields[4].to_owned(),
                command: fields[5].to_owned(),
            })
        })
        .collect())
}

fn get_services() -> AppResult<Vec<ServiceInfo>> {
    let output = run_command(&[
        "systemctl",
        "list-units",
        "--type=service",
        "--state=running",
        "--no-legend",
        "--no-pager",
    ])?;
    Ok(output
        .lines()
        .filter_map(|line| {
            let fields: Vec<_> = line
                .trim_start()
                .splitn(5, char::is_whitespace)
                .filter(|s| !s.is_empty())
                .collect();
            if fields.len() < 4 {
                return None;
            }
            Some(ServiceInfo {
                unit: fields[0].to_owned(),
                state: format!("{}/{}", fields[2], fields[3]),
                description: fields.get(4).copied().unwrap_or_default().to_owned(),
            })
        })
        .collect())
}

fn get_interfaces() -> AppResult<Vec<InterfaceInfo>> {
    let output = std::fs::read_to_string("/proc/net/dev")
        .map_err(|error| format!("Impossible de lire /proc/net/dev : {error}"))?;
    Ok(output
        .lines()
        .skip(2)
        .filter_map(|line| {
            let (name, counters) = line.split_once(':')?;
            let fields: Vec<_> = counters.split_whitespace().collect();
            if fields.len() < 9 {
                return None;
            }
            Some(InterfaceInfo {
                name: name.trim().to_owned(),
                rx_bytes: fields[0].parse().ok()?,
                tx_bytes: fields[8].parse().ok()?,
            })
        })
        .collect())
}

fn get_connections() -> AppResult<Vec<String>> {
    Ok(run_command(&["ss", "-H", "-tunap", "state", "established"])?
        .lines()
        .map(str::trim)
        .filter(|line| !line.is_empty())
        .map(str::to_owned)
        .collect())
}

fn get_flatpak_apps() -> AppResult<(bool, Vec<FlatpakApp>)> {
    if !find_executable("flatpak") {
        return Ok((false, Vec::new()));
    }
    let output = run_command(&["flatpak", "list", "--app", "--columns=application,name"])?;
    let mut apps = Vec::new();
    for line in output.lines() {
        let mut fields = line
            .trim_start()
            .splitn(2, char::is_whitespace)
            .filter(|s| !s.is_empty());
        let Some(id) = fields.next() else {
            continue;
        };
        let name = fields.next().unwrap_or(id);
        let permissions = match run_command(&["flatpak", "info", "--show-permissions", id]) {
            Ok(value) if !value.trim().is_empty() => value.trim().to_owned(),
            Ok(_) => "Aucune permission déclarée".to_owned(),
            Err(error) => format!("Lecture impossible : {error}"),
        };
        apps.push(FlatpakApp {
            id: id.to_owned(),
            name: name.to_owned(),
            permissions,
        });
    }
    Ok((true, apps))
}

fn snapshot() -> AppResult<Snapshot> {
    let (flatpak_available, flatpak_apps) = get_flatpak_apps()?;
    Ok(Snapshot {
        processes: get_processes()?,
        services: get_services()?,
        interfaces: get_interfaces()?,
        connections: get_connections()?,
        flatpak_available,
        flatpak_apps,
    })
}

fn json_string(value: &str) -> String {
    let mut escaped = String::with_capacity(value.len() + 2);
    escaped.push('"');
    for character in value.chars() {
        match character {
            '"' => escaped.push_str("\\\""),
            '\\' => escaped.push_str("\\\\"),
            '\u{08}' => escaped.push_str("\\b"),
            '\u{0c}' => escaped.push_str("\\f"),
            '\n' => escaped.push_str("\\n"),
            '\r' => escaped.push_str("\\r"),
            '\t' => escaped.push_str("\\t"),
            character if character <= '\u{1f}' => {
                write!(escaped, "\\u{:04x}", character as u32).expect("writing to String");
            }
            character => escaped.push(character),
        }
    }
    escaped.push('"');
    escaped
}

fn json_object(fields: &[(&str, String)]) -> String {
    let entries = fields
        .iter()
        .map(|(key, value)| format!("{}:{value}", json_string(key)))
        .collect::<Vec<_>>()
        .join(",");
    format!("{{{entries}}}")
}

fn snapshot_json(data: &Snapshot) -> String {
    let processes = data
        .processes
        .iter()
        .map(|item| {
            json_object(&[
                ("pid", json_string(&item.pid)),
                ("user", json_string(&item.user)),
                ("state", json_string(&item.state)),
                ("cpu", json_string(&item.cpu)),
                ("memory", json_string(&item.memory)),
                ("command", json_string(&item.command)),
            ])
        })
        .collect::<Vec<_>>()
        .join(",");
    let services = data
        .services
        .iter()
        .map(|item| {
            json_object(&[
                ("unit", json_string(&item.unit)),
                ("state", json_string(&item.state)),
                ("description", json_string(&item.description)),
            ])
        })
        .collect::<Vec<_>>()
        .join(",");
    let interfaces = data
        .interfaces
        .iter()
        .map(|item| {
            format!(
                "{{\"name\":{},\"rx_bytes\":{},\"tx_bytes\":{}}}",
                json_string(&item.name),
                item.rx_bytes,
                item.tx_bytes
            )
        })
        .collect::<Vec<_>>()
        .join(",");
    let connections = data
        .connections
        .iter()
        .map(|item| json_string(item))
        .collect::<Vec<_>>()
        .join(",");
    let flatpak_apps = data
        .flatpak_apps
        .iter()
        .map(|item| {
            json_object(&[
                ("id", json_string(&item.id)),
                ("name", json_string(&item.name)),
                ("permissions", json_string(&item.permissions)),
            ])
        })
        .collect::<Vec<_>>()
        .join(",");
    format!(
        "{{\"processes\":[{processes}],\"services\":[{services}],\
         \"interfaces\":[{interfaces}],\"connections\":[{connections}],\
         \"flatpak_available\":{},\"flatpak_apps\":[{flatpak_apps}]}}",
        data.flatpak_available
    )
}

fn print_table(title: &str, rows: &[Vec<(&str, String)>]) {
    println!("\n{title}");
    if rows.is_empty() {
        println!("  Aucun élément trouvé.");
        return;
    }
    for row in rows {
        println!(
            "  {}",
            row.iter()
                .map(|(label, value)| format!("{label}: {value}"))
                .collect::<Vec<_>>()
                .join(" | ")
        );
    }
}

fn process_rows(processes: &[ProcessInfo]) -> Vec<Vec<(&'static str, String)>> {
    processes
        .iter()
        .map(|item| {
            vec![
                ("PID", item.pid.clone()),
                ("Utilisateur", item.user.clone()),
                ("État", item.state.clone()),
                ("CPU %", item.cpu.clone()),
                ("Mémoire %", item.memory.clone()),
                ("Commande", item.command.clone()),
            ]
        })
        .collect()
}

fn service_rows(services: &[ServiceInfo]) -> Vec<Vec<(&'static str, String)>> {
    services
        .iter()
        .map(|item| {
            vec![
                ("Service", item.unit.clone()),
                ("État", item.state.clone()),
                ("Description", item.description.clone()),
            ]
        })
        .collect()
}

fn print_network(interfaces: &[InterfaceInfo]) {
    println!("\nTrafic réseau cumulé par interface (octets)");
    for interface in interfaces {
        let loopback_note = if interface.name == "lo" {
            " (boucle locale)"
        } else {
            ""
        };
        println!(
            "  {}{}: reçus={} envoyés={}",
            interface.name, loopback_note, interface.rx_bytes, interface.tx_bytes
        );
    }
}

fn print_permissions(available: bool, apps: &[FlatpakApp]) {
    println!("\nPermissions des applications Flatpak");
    if !available {
        println!("  Flatpak n’est pas installé ; ces permissions ne sont pas disponibles.");
    } else if apps.is_empty() {
        println!("  Aucune application Flatpak installée.");
    } else {
        for app in apps {
            println!("  {} ({})", app.name, app.id);
            for line in app.permissions.lines() {
                println!("    {line}");
            }
        }
    }
}

fn command_scan() -> AppResult<()> {
    let data = snapshot()?;
    print_table(
        "Processus système (triés par utilisation CPU)",
        &process_rows(&data.processes),
    );
    print_table("Services actifs", &service_rows(&data.services));
    print_network(&data.interfaces);
    println!("\nConnexions réseau établies");
    if data.connections.is_empty() {
        println!("  Aucune.");
    } else {
        for connection in &data.connections {
            println!("  {connection}");
        }
    }
    print_permissions(data.flatpak_available, &data.flatpak_apps);
    println!(
        "\nRemarque : Linux ne fournit pas une liste universelle des permissions \
         de toutes les applications. Les permissions affichées ici concernent Flatpak."
    );
    Ok(())
}

fn transmitted_bytes_by_interface() -> AppResult<Vec<(String, u64)>> {
    Ok(get_interfaces()?
        .into_iter()
        .filter(|interface| interface.name != "lo")
        .map(|interface| (interface.name, interface.tx_bytes))
        .collect())
}

fn command_monitor(interval: u64, threshold_mib: f64) -> AppResult<()> {
    let mut previous = transmitted_bytes_by_interface()?;
    println!(
        "Surveillance du trafic sortant (interfaces hors boucle locale), \
         seuil={threshold_mib} MiB/{interval}s. Ctrl+C pour arrêter.",
        threshold_mib = threshold_mib
    );
    io::stdout()
        .flush()
        .map_err(|error| format!("Impossible d’écrire sur la sortie : {error}"))?;
    let threshold_bytes = threshold_mib * 1024.0 * 1024.0;
    loop {
        thread::sleep(Duration::from_secs(interval));
        let current = transmitted_bytes_by_interface()?;
        for (name, sent) in &current {
            let old = previous
                .iter()
                .find_map(|(old_name, old_sent)| (old_name == name).then_some(*old_sent))
                .unwrap_or(*sent);
            let delta = sent.saturating_sub(old);
            let mib = delta as f64 / (1024.0 * 1024.0);
            let prefix = if delta as f64 >= threshold_bytes {
                "ALERTE"
            } else {
                "Info"
            };
            println!(
                "{prefix} [{name}] : {mib:.2} MiB envoyés en {interval} s."
            );
            io::stdout()
                .flush()
                .map_err(|error| format!("Impossible d’écrire sur la sortie : {error}"))?;
        }
        previous = current;
    }
}

fn valid_service_unit(unit: &str) -> bool {
    let Some(name) = unit.strip_suffix(".service") else {
        return false;
    };
    let mut characters = name.chars();
    characters
        .next()
        .is_some_and(|first| first.is_ascii_alphanumeric() || first == '_')
        && characters.all(|character| {
            character.is_ascii_alphanumeric()
                || matches!(character, '_' | '.' | '@' | ':' | '-')
        })
}

fn is_root() -> AppResult<bool> {
    let status = std::fs::read_to_string("/proc/self/status")
        .map_err(|error| format!("Impossible de lire l’identité du processus : {error}"))?;
    let uid_line = status
        .lines()
        .find(|line| line.starts_with("Uid:"))
        .ok_or_else(|| "Identifiant utilisateur absent de /proc/self/status.".to_owned())?;
    let effective_uid = uid_line
        .split_whitespace()
        .nth(2)
        .ok_or_else(|| "Identifiant utilisateur effectif indisponible.".to_owned())?;
    Ok(effective_uid == "0")
}

fn command_stop_service(unit: &str) -> AppResult<()> {
    if !valid_service_unit(unit) {
        return Err(
            "Nom invalide : seules les unités systemd .service sont acceptées.".to_owned(),
        );
    }
    let active = command_output(&["systemctl", "is-active", "--quiet", unit])?;
    if !active.status.success() {
        return Err(format!("Le service {unit} n’est pas actif ou n’existe pas."));
    }
    println!(
        "Arrêter {unit} peut interrompre une fonction système. \
         Le service ne sera pas désactivé au prochain démarrage."
    );
    print!("Tapez exactement « {unit} » pour confirmer : ");
    io::stdout()
        .flush()
        .map_err(|error| format!("Impossible d’écrire sur la sortie : {error}"))?;
    let mut confirmation = String::new();
    io::stdin()
        .read_line(&mut confirmation)
        .map_err(|error| format!("Impossible de lire la confirmation : {error}"))?;
    if confirmation.trim_end_matches(|character| character == '\r' || character == '\n') != unit {
        println!("Annulé ; aucun changement effectué.");
        return Ok(());
    }

    let command: Vec<&str> = if is_root()? {
        vec!["systemctl", "stop", unit]
    } else {
        if !find_executable("pkexec") {
            return Err("pkexec est requis pour demander les privilèges administrateur.".to_owned());
        }
        vec!["pkexec", "systemctl", "stop", unit]
    };
    run_command(&command)?;
    println!("Service arrêté : {unit}");
    Ok(())
}

fn positive_integer(value: &str) -> AppResult<u64> {
    let parsed = value
        .parse::<u64>()
        .map_err(|_| "une valeur entière positive est requise".to_owned())?;
    if parsed == 0 {
        return Err("la valeur doit être supérieure à zéro".to_owned());
    }
    Ok(parsed)
}

fn positive_float(value: &str) -> AppResult<f64> {
    let parsed = value
        .parse::<f64>()
        .map_err(|_| "une valeur positive est requise".to_owned())?;
    if !parsed.is_finite() || parsed <= 0.0 {
        return Err("la valeur doit être supérieure à zéro".to_owned());
    }
    Ok(parsed)
}

fn usage() {
    println!(
        "Usage : nonoos-security <commande> [options]\n\
         \nCommandes :\n\
         \x20 scan                   afficher un état général en lecture seule\n\
         \x20 snapshot               produire l’inventaire au format JSON\n\
         \x20 processes              lister les processus\n\
         \x20 services               lister les services actifs\n\
         \x20 network                afficher les compteurs réseau\n\
         \x20 connections            lister les connexions établies\n\
         \x20 permissions            afficher les permissions Flatpak\n\
         \x20 monitor [options]      surveiller le volume sortant\n\
         \x20 stop-service <unité>   arrêter un service après confirmation\n\
         \nOptions monitor :\n\
         \x20 --interval SECONDES    intervalle en secondes (défaut : 5)\n\
         \x20 --threshold-mib MIB    seuil par intervalle en MiB (défaut : 10)"
    );
}

fn parse_monitor_options(arguments: &[String]) -> AppResult<(u64, f64)> {
    let mut interval = 5;
    let mut threshold_mib = 10.0;
    let mut index = 0;
    while index < arguments.len() {
        let option = arguments[index].as_str();
        let value = arguments
            .get(index + 1)
            .ok_or_else(|| format!("une valeur est requise pour {option}"))?;
        match option {
            "--interval" => interval = positive_integer(value)?,
            "--threshold-mib" => threshold_mib = positive_float(value)?,
            _ => return Err(format!("option inconnue : {option}")),
        }
        index += 2;
    }
    Ok((interval, threshold_mib))
}

fn execute(arguments: &[String]) -> AppResult<()> {
    let Some(command) = arguments.first().map(String::as_str) else {
        usage();
        return Err("une commande est requise".to_owned());
    };
    match command {
        "-h" | "--help" => {
            usage();
            Ok(())
        }
        "scan" if arguments.len() == 1 => command_scan(),
        "snapshot" if arguments.len() == 1 => {
            println!("{}", snapshot_json(&snapshot()?));
            Ok(())
        }
        "processes" if arguments.len() == 1 => {
            print_table(
                "Processus système (triés par utilisation CPU)",
                &process_rows(&get_processes()?),
            );
            Ok(())
        }
        "services" if arguments.len() == 1 => {
            print_table("Services actifs", &service_rows(&get_services()?));
            Ok(())
        }
        "network" if arguments.len() == 1 => {
            print_network(&get_interfaces()?);
            Ok(())
        }
        "connections" if arguments.len() == 1 => {
            println!("Connexions réseau établies");
            let connections = get_connections()?;
            if connections.is_empty() {
                println!("  Aucune.");
            } else {
                for connection in connections {
                    println!("  {connection}");
                }
            }
            Ok(())
        }
        "permissions" if arguments.len() == 1 => {
            let (available, apps) = get_flatpak_apps()?;
            print_permissions(available, &apps);
            Ok(())
        }
        "monitor" => {
            let (interval, threshold_mib) = parse_monitor_options(&arguments[1..])?;
            command_monitor(interval, threshold_mib)
        }
        "stop-service" if arguments.len() == 2 => command_stop_service(&arguments[1]),
        "scan" | "snapshot" | "processes" | "services" | "network" | "connections"
        | "permissions" | "stop-service" => Err("nombre d’arguments invalide".to_owned()),
        _ => Err(format!("commande inconnue : {command}")),
    }
}

fn main() {
    let arguments: Vec<String> = env::args().skip(1).collect();
    if let Err(error) = execute(&arguments) {
        eprintln!("Erreur : {error}");
        std::process::exit(1);
    }
}

#[cfg(test)]
mod tests {
    use super::{json_string, positive_float, positive_integer, valid_service_unit};

    #[test]
    fn escapes_json_control_characters_and_quotes() {
        assert_eq!(json_string("a\"\\\n\t"), "\"a\\\"\\\\\\n\\t\"");
    }

    #[test]
    fn validates_service_unit_names() {
        assert!(valid_service_unit("ssh.service"));
        assert!(valid_service_unit("name@instance.service"));
        assert!(!valid_service_unit(".service"));
        assert!(!valid_service_unit("ssh.socket"));
        assert!(!valid_service_unit("bad/name.service"));
    }

    #[test]
    fn validates_monitor_options() {
        assert_eq!(positive_integer("5").unwrap(), 5);
        assert!(positive_integer("0").is_err());
        assert_eq!(positive_float("0.5").unwrap(), 0.5);
        assert!(positive_float("NaN").is_err());
    }
}
