const state = {
  data: null,
  section: "home",
  list: [],
  selected: null,
  history: [],
  graphTab: "Graph",
};

const els = {
  listTitle: document.getElementById("listTitle"),
  listSubtitle: document.getElementById("listSubtitle"),
  listView: document.getElementById("listView"),
  detailTitle: document.getElementById("detailTitle"),
  detailSubtitle: document.getElementById("detailSubtitle"),
  detailActions: document.getElementById("detailActions"),
  detailTabs: document.getElementById("detailTabs"),
  detailBody: document.getElementById("detailBody"),
  helperArea: document.getElementById("helperArea"),
  pathLabel: document.getElementById("pathLabel"),
  backButton: document.getElementById("backButton"),
  homeButton: document.getElementById("homeButton"),
};

const helperFields = {
  ped: ["Start quantity", "End quantity", "Start price", "End price"],
  pes: ["Start quantity", "End quantity", "Start price", "End price"],
  midpoint: ["Start quantity", "End quantity", "Start price", "End price"],
  cross: ["Start qty of good A", "End qty of good A", "Start price of good B", "End price of good B"],
  income: ["Start quantity", "End quantity", "Start income", "End income"],
  total_revenue: ["Price", "Quantity"],
  marginal_revenue: ["Initial total revenue", "New total revenue", "Initial quantity", "New quantity"],
  total_cost: ["Fixed cost", "Variable cost"],
  profit: ["Price", "Quantity", "Average total cost"],
  accounting_economic_profit: ["Total revenue", "Explicit costs", "Implicit costs"],
  average_costs: ["Fixed cost", "Variable cost", "Quantity"],
  marginal_cost: ["Initial total cost", "New total cost", "Initial quantity", "New quantity"],
  tax_revenue: ["Tax per unit", "Quantity after tax"],
  dwl: ["Base", "Height"],
  mrp: ["Marginal product", "Marginal revenue"],
};

document.querySelectorAll(".sidebar button[data-section]").forEach((button) => {
  button.addEventListener("click", () => selectSection(button.dataset.section, true));
});
els.homeButton.addEventListener("click", () => selectSection("home", true));
els.backButton.addEventListener("click", goBack);

fetch("../generated/apmicro_content.json")
  .then((response) => response.json())
  .then((data) => {
    state.data = data;
    selectSection("home", false);
  })
  .catch((error) => {
    els.listTitle.textContent = "Preview load failed";
    els.detailTitle.textContent = "Error";
    els.detailBody.textContent = String(error);
  });

function pushHistory() {
  state.history.push({
    section: state.section,
    selectedId: state.selected?.id ?? null,
    graphTab: state.graphTab,
  });
}

function goBack() {
  const previous = state.history.pop();
  if (!previous) return;
  state.graphTab = previous.graphTab || "Graph";
  selectSection(previous.section, false, previous.selectedId);
}

function selectSection(section, push = true, selectedId = null) {
  if (!state.data) return;
  if (push && state.section) pushHistory();
  state.section = section;
  state.graphTab = "Graph";

  if (section === "home") {
    state.list = [
      { id: "units", title: "Units", subtitle: "Unit overviews, topics, and links" },
      { id: "vocabulary", title: "Vocabulary", subtitle: "By unit, A-Z, and category lookup" },
      { id: "graphs", title: "Graphs", subtitle: "18 graph references with subpages" },
      { id: "formulas", title: "Formulas", subtitle: "15 formula cards and helpers" },
      { id: "structures", title: "Structures", subtitle: "Market structure comparisons" },
      { id: "quick_review", title: "Quick Review", subtitle: "Compact reference sheets" },
      { id: "exam_cram", title: "Exam Cram", subtitle: "Fast high-yield pages" },
      { id: "reference", title: "Reference", subtitle: "Shift sheets and FRQ reminders" },
      { id: "audit", title: "Source Audit", subtitle: "Desktop content source inventory" },
    ];
    state.selected = state.list[0];
    renderList("Home", "Preview sections", state.list, (item) => selectSection(item.id, true));
    renderHomeDetail();
    return;
  }

  const catalog = {
    units: ["units", "AP units", "title", "body"],
    vocabulary: ["vocabulary", "Rich term records", "term", "short_definition"],
    graphs: ["graphs", "Graph references", "title", "graph_type"],
    formulas: ["formulas", "Formula cards", "title", "formula"],
    structures: ["structures", "Market structure comparisons", "title", "summary"],
    quick_review: ["quick_review", "Quick review pages", "title", "body"],
    exam_cram: ["exam_cram", "High-yield cram pages", "title", "body"],
    reference: ["reference", "Cheat sheet pages", "title", "body"],
    audit: ["audit", "Source scan pages", "title", "body"],
  };

  const [key, subtitle, titleField, subtitleField] = catalog[section];
  state.list = state.data[key];
  state.selected = state.list.find((item) => item.id === selectedId) || state.list[0] || null;
  renderList(capitalize(section.replace("_", " ")), subtitle, state.list, (item) => {
    state.selected = item;
    renderDetail();
    renderList(capitalize(section.replace("_", " ")), subtitle, state.list, () => {});
  }, titleField, subtitleField);
  renderDetail();
}

function renderList(title, subtitle, list, onSelect, titleField = "title", subtitleField = "subtitle") {
  els.listTitle.textContent = title;
  els.listSubtitle.textContent = subtitle;
  els.listView.innerHTML = "";
  els.pathLabel.textContent = `${capitalize(state.section.replace("_", " "))}${state.selected ? " / " + (state.selected.title || state.selected.term) : ""}`;

  list.forEach((item) => {
    const div = document.createElement("button");
    div.className = "list-item" + (state.selected === item ? " active" : "");
    div.innerHTML = `
      <div class="list-item-title">${escapeHtml(item[titleField] || item.term || item.id)}</div>
      <div class="list-item-subtitle">${escapeHtml(item[subtitleField] || item.category || "")}</div>
    `;
    div.addEventListener("click", () => {
      if (state.section === "home") {
        onSelect(item);
        return;
      }
      state.selected = item;
      renderDetail();
      renderList(title, subtitle, list, onSelect, titleField, subtitleField);
    });
    els.listView.appendChild(div);
  });
}

function renderHomeDetail() {
  els.detailTitle.textContent = "AP Microeconomics";
  els.detailSubtitle.textContent = "Browser preview for the native fx-CG50 add-in";
  els.detailActions.innerHTML = "";
  els.detailTabs.innerHTML = "";
  els.helperArea.innerHTML = "";
  els.detailBody.textContent =
    "This preview reads the same generated content used by the native add-in.\n\n" +
    "Use the left column to inspect units, vocabulary, graphs, formulas, structures, quick review pages, exam cram pages, and source audit output.\n\n" +
    "The official calculator-like emulator option is CASIO fx-CG Manager PLUS. This browser preview is the local no-transfer fallback for content, link, and formula inspection.";
}

function renderDetail() {
  if (!state.selected) {
    renderHomeDetail();
    return;
  }

  const item = state.selected;
  els.detailActions.innerHTML = "";
  els.detailTabs.innerHTML = "";
  els.helperArea.innerHTML = "";

  if (state.section === "units") {
    els.detailTitle.textContent = item.title;
    els.detailSubtitle.textContent = "Unit overview";
    addAction("Related Topics", () => showDerivedList(`Topics in ${item.title}`, state.data.topics.filter((topic) => topic.unit_id === item.id), "title", "related_graphs"));
    addAction("Unit Vocabulary", () => showDerivedList(`Vocabulary in ${item.title}`, state.data.vocabulary.filter((term) => term.unit_id === item.id), "term", "category"));
    els.detailBody.textContent = item.body;
    return;
  }

  if (state.section === "vocabulary") {
    els.detailTitle.textContent = item.term;
    els.detailSubtitle.textContent = item.category;
    addAction("Related Graph", () => jumpToTitle("graphs", item.graph_name || splitCsv(item.related_graphs)[0]));
    addAction("Related Formula", () => jumpToTitle("formulas", splitCsv(item.related_formulas)[0]));
    els.detailBody.textContent = [
      `Short definition\n${item.short_definition}`,
      `Longer explanation\n${item.long_definition}`,
      `Unit / Topic\n${findById(state.data.units, item.unit_id)?.title || item.unit_id}\n${findById(state.data.topics, item.topic_id)?.title || item.topic_id}`,
      `Used for\n${item.used_for}`,
      `Question types\n${item.question_types}`,
      `Graph location\n${item.graph_name}\n${item.graph_kind}\n${item.graph_where}\n${item.graph_meaning}\n${item.graph_effect}`,
      `Related formulas\n${item.related_formulas}`,
      `Related terms\n${item.related_terms}`,
      `Common confusion\n${item.confusion}`,
      `AP exam tip\n${item.exam_tip}`,
    ].join("\n\n");
    return;
  }

  if (state.section === "graphs") {
    const tabs = {
      Graph: `${item.title}\n\nGraph type: ${item.graph_type}\n\n${item.overview_page}\n\nRelated terms\n${item.related_terms || "None"}\n\nRelated formulas\n${item.related_formulas || "None"}`,
      Labels: item.labels_page,
      Shifts: item.shifts_page,
      "Reading Guide": item.guide_page,
      "Common Questions": item.questions_page,
      Mistakes: item.mistakes_page,
    };
    els.detailTitle.textContent = item.title;
    els.detailSubtitle.textContent = "Graph reference";
    Object.keys(tabs).forEach((tab) => addTab(tab, () => {
      state.graphTab = tab;
      renderDetail();
    }, state.graphTab === tab));
    addAction("Related Term", () => jumpToTitle("vocabulary", splitCsv(item.related_terms)[0], "term"));
    addAction("Related Formula", () => jumpToTitle("formulas", splitCsv(item.related_formulas)[0]));
    els.detailBody.textContent = tabs[state.graphTab] || tabs.Graph;
    return;
  }

  if (state.section === "formulas") {
    els.detailTitle.textContent = item.title;
    els.detailSubtitle.textContent = "Formula card";
    addAction("Related Graph", () => jumpToTitle("graphs", splitCsv(item.related_graphs)[0]));
    if (item.helper) renderHelper(item);
    els.detailBody.textContent = item.body + (item.related_terms ? `\n\nRelated terms\n${item.related_terms}` : "");
    return;
  }

  if (state.section === "structures") {
    els.detailTitle.textContent = item.title;
    els.detailSubtitle.textContent = "Market structure comparison";
    addAction("Related Graph", () => jumpToTitle("graphs", splitCsv(item.related_graphs)[0]) || jumpToId("graphs", splitCsv(item.related_graphs)[0]));
    els.detailBody.textContent = `${item.summary}\n\n${item.body}\n\nRelated terms\n${item.related_terms}`;
    return;
  }

  els.detailTitle.textContent = item.title;
  els.detailSubtitle.textContent = capitalize(state.section.replace("_", " "));
  els.detailBody.textContent = item.body;
}

function showDerivedList(title, list, titleField, subtitleField) {
  state.section = title;
  state.list = list;
  state.selected = list[0] || null;
  renderList(title, "Derived list", list, () => {}, titleField, subtitleField);
  if (state.selected) {
    if (titleField === "term") {
      const saveSection = state.section;
      state.section = "vocabulary";
      renderDetail();
      state.section = saveSection;
    } else {
      els.detailTitle.textContent = state.selected[titleField];
      els.detailSubtitle.textContent = state.selected[subtitleField] || "";
      els.detailBody.textContent = state.selected.body || state.selected.short_definition || "";
    }
  }
}

function jumpToTitle(section, title, field = "title") {
  if (!title) return false;
  const list = state.data[section];
  const item = list.find((entry) => entry[field] === title);
  if (!item) return false;
  pushHistory();
  state.section = section;
  state.selected = item;
  state.list = list;
  renderList(capitalize(section.replace("_", " ")), "Linked content", list, () => {}, field, field === "term" ? "category" : "body");
  renderDetail();
  return true;
}

function jumpToId(section, id) {
  if (!id) return false;
  const list = state.data[section];
  const item = list.find((entry) => entry.id === id);
  if (!item) return false;
  pushHistory();
  state.section = section;
  state.selected = item;
  state.list = list;
  renderList(capitalize(section.replace("_", " ")), "Linked content", list, () => {});
  renderDetail();
  return true;
}

function addAction(label, handler) {
  const button = document.createElement("button");
  button.className = "action-button";
  button.textContent = label;
  button.addEventListener("click", handler);
  els.detailActions.appendChild(button);
}

function addTab(label, handler, active) {
  const button = document.createElement("button");
  button.className = "tab-button" + (active ? " active" : "");
  button.textContent = label;
  button.addEventListener("click", handler);
  els.detailTabs.appendChild(button);
}

function renderHelper(item) {
  const labels = helperFields[item.helper];
  if (!labels) return;
  els.helperArea.innerHTML = `<h3>Calculator Helper</h3>`;
  const grid = document.createElement("div");
  grid.className = "helper-grid";
  labels.forEach((label, index) => {
    const wrapper = document.createElement("label");
    wrapper.innerHTML = `${escapeHtml(label)}<input type="number" step="any" data-index="${index}">`;
    grid.appendChild(wrapper);
  });
  const button = document.createElement("button");
  button.textContent = "Compute";
  const result = document.createElement("div");
  result.className = "helper-result";
  button.addEventListener("click", () => {
    const values = [...grid.querySelectorAll("input")].map((input) => Number(input.value));
    result.textContent = computeHelper(item.helper, values);
  });
  els.helperArea.appendChild(grid);
  els.helperArea.appendChild(button);
  els.helperArea.appendChild(result);
}

function computeHelper(helper, values) {
  if (values.some((value) => Number.isNaN(value))) return "Enter all required numeric values.";
  const nonNegative = (value, label) => value >= 0 && Number.isFinite(value) ? null : `${label} must be non-negative.`;
  const finite = (value, label) => Number.isFinite(value) ? null : `${label} overflowed or became invalid.`;
  const [a, b, c, d] = values;

  if (["ped", "pes", "midpoint"].includes(helper)) {
    const err = nonNegative(a, "Start quantity") || nonNegative(b, "End quantity") || nonNegative(c, "Start price") || nonNegative(d, "End price");
    if (err) return err;
    if (((a + b) / 2) === 0 || ((c + d) / 2) === 0 || (d - c) === 0) return "Midpoint averages and the change term cannot create a zero denominator.";
    const raw = ((b - a) / ((a + b) / 2)) / ((d - c) / ((c + d) / 2));
    if (finite(raw, "Elasticity")) return finite(raw, "Elasticity");
    return `Elasticity = ${raw.toFixed(4)}\nAbsolute value = ${Math.abs(raw).toFixed(4)}`;
  }
  if (helper === "cross") {
    const err = nonNegative(a, "Start qty A") || nonNegative(b, "End qty A") || nonNegative(c, "Start price B") || nonNegative(d, "End price B");
    if (err) return err;
    if (((a + b) / 2) === 0 || ((c + d) / 2) === 0 || (d - c) === 0) return "Midpoint averages and the change term cannot create a zero denominator.";
    const raw = ((b - a) / ((a + b) / 2)) / ((d - c) / ((c + d) / 2));
    return `XED = ${raw.toFixed(4)}\n${raw > 0 ? "Positive suggests substitutes." : raw < 0 ? "Negative suggests complements." : "Near zero suggests weak relation."}`;
  }
  if (helper === "income") {
    const err = nonNegative(a, "Start quantity") || nonNegative(b, "End quantity") || nonNegative(c, "Start income") || nonNegative(d, "End income");
    if (err) return err;
    if (((a + b) / 2) === 0 || ((c + d) / 2) === 0 || (d - c) === 0) return "Midpoint averages and the income change cannot create a zero denominator.";
    const raw = ((b - a) / ((a + b) / 2)) / ((d - c) / ((c + d) / 2));
    return `YED = ${raw.toFixed(4)}\n${raw >= 0 ? "Positive suggests a normal good." : "Negative suggests an inferior good."}`;
  }
  if (helper === "total_revenue") return `TR = ${(a * b).toFixed(4)}`;
  if (helper === "marginal_revenue" || helper === "marginal_cost") {
    if ((d - c) === 0) return "Quantity change cannot be zero.";
    return `${helper === "marginal_revenue" ? "MR" : "MC"} = ${((b - a) / (d - c)).toFixed(4)}`;
  }
  if (helper === "total_cost") return `TC = ${(a + b).toFixed(4)}`;
  if (helper === "profit") return `Profit = ${(a * b - c * b).toFixed(4)}`;
  if (helper === "accounting_economic_profit") return `Accounting profit = ${(a - b).toFixed(4)}\nEconomic profit = ${(a - b - c).toFixed(4)}`;
  if (helper === "average_costs") {
    if (c <= 0) return "Quantity must be greater than zero.";
    return `AFC = ${(a / c).toFixed(4)}\nAVC = ${(b / c).toFixed(4)}\nATC = ${((a + b) / c).toFixed(4)}`;
  }
  if (helper === "tax_revenue") return `Tax revenue = ${(a * b).toFixed(4)}`;
  if (helper === "dwl") return `DWL = ${(0.5 * a * b).toFixed(4)}`;
  if (helper === "mrp") return `MRP = ${(a * b).toFixed(4)}`;
  return "No browser helper is configured for this formula.";
}

function splitCsv(value) {
  return (value || "").split(",").map((part) => part.trim()).filter(Boolean);
}

function findById(list, id) {
  return list.find((item) => item.id === id);
}

function capitalize(text) {
  return text.charAt(0).toUpperCase() + text.slice(1);
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}
