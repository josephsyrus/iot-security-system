lucide.createIcons();

const firebaseURL =
  "https://iot-security-system-dadf2-default-rtdb.asia-southeast1.firebasedatabase.app/logs.json";
const tbody = document.getElementById("logTable");
const loadingState = document.getElementById("loadingState");
const emptyState = document.getElementById("emptyState");
const lastUpdatedLabel = document.getElementById("lastUpdated");
const lastUpdatedContainer = document.getElementById("lastUpdatedContainer");

function formatTimestamp(ts) {
  if (!ts) return { date: "N/A", time: "N/A" };
  const numTs = Number(ts);
  const dateObj = !isNaN(numTs) ? new Date(numTs) : new Date(ts);
  return {
    date: dateObj.toLocaleDateString(undefined, {
      month: "short",
      day: "numeric",
      year: "numeric",
    }),
    time: dateObj.toLocaleTimeString(undefined, {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    }),
  };
}

function getEventBadge(eventText) {
  const text = (eventText || "").toLowerCase();
  let badgeClass = "badge-default";
  let icon = "activity";

  if (
    text.includes("intruder") ||
    text.includes("denied") ||
    text.includes("unauthorized") ||
    text.includes("fail")
  ) {
    badgeClass = "badge-danger";
    icon = "alert-triangle";
  } else if (
    text.includes("grant") ||
    text.includes("auth") ||
    text.includes("success") ||
    text.includes("allow")
  ) {
    badgeClass = "badge-success";
    icon = "check-circle-2";
  }

  return `<span class="badge ${badgeClass}"><i data-lucide="${icon}"></i>${eventText || "Unknown"}</span>`;
}

async function fetchLogs() {
  try {
    const response = await fetch(firebaseURL);
    const data = await response.json();

    loadingState.classList.add("hidden");
    lastUpdatedContainer.classList.remove("error");

    const now = new Date();
    lastUpdatedLabel.innerText = `Updated ${now.toLocaleTimeString()}`;

    if (!data) {
      tbody.innerHTML = "";
      emptyState.classList.remove("hidden");
      return;
    }

    emptyState.classList.add("hidden");

    const logsArray = Object.values(data).reverse();

    const rowsHTML = logsArray
      .map((log) => {
        const { date, time } = formatTimestamp(log.timestamp);
        const badgeHTML = getEventBadge(log.event);
        const name = log.name || "Unknown Identity";
        const uid = log.card_uid || "—";

        return `
      <tr>
        <td>
          <span class="time-primary">${time}</span>
          <span class="time-secondary">${date}</span>
        </td>
        <td>
          <div class="identity-cell">
            <div class="avatar-circle">
              <i data-lucide="user"></i>
            </div>
            <span class="identity-name">${name}</span>
          </div>
        </td>
        <td class="center">
          <code class="uid-code">${uid}</code>
        </td>
        <td class="right">
          ${badgeHTML}
        </td>
      </tr>`;
      })
      .join("");

    tbody.innerHTML = rowsHTML;
    lucide.createIcons();
  } catch (err) {
    console.error("Error fetching logs:", err);
    lastUpdatedLabel.innerText = "Connection Error";
    lastUpdatedContainer.classList.add("error");

    if (tbody.innerHTML === "") {
      loadingState.classList.remove("hidden");
      loadingState.innerHTML = `
        <i data-lucide="wifi-off" class="error-icon"></i>
        <p class="state-text-main" style="color: var(--danger-light);">Failed to connect</p>
        <p class="state-text-sub">Check console for details.</p>`;
      lucide.createIcons();
    }
  }
}

fetchLogs();
setInterval(fetchLogs, 5000);
