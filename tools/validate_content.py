from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path


CALC_ROOT = Path(__file__).resolve().parent.parent
CONTENT_JSON = CALC_ROOT / "generated" / "apmicro_content.json"
REPORT_TXT = CALC_ROOT / "generated" / "validation_report.txt"
GRAPH_MODEL_C = CALC_ROOT / "src" / "graph_model.c"
GENERATED_HEADER = CALC_ROOT / "src" / "generated" / "apmicro_content.h"

VALID_CATEGORIES = {
    "core concept",
    "graph label",
    "graph area",
    "graph point",
    "formula variable",
    "elasticity term",
    "cost/revenue term",
    "market structure term",
    "labor market term",
    "government intervention term",
    "externality term",
    "efficiency/welfare term",
}

VALID_HELPERS = {
    "",
    "ped",
    "pes",
    "cross",
    "income",
    "midpoint",
    "total_revenue",
    "marginal_revenue",
    "total_cost",
    "profit",
    "accounting_economic_profit",
    "average_costs",
    "marginal_cost",
    "tax_revenue",
    "dwl",
    "mrp",
    "hiring_rule",
}

VALID_VOCAB_SOURCES = {"manual", "extracted", "merged"}

REJECTED_VOCAB_TERMS = {
    "availability of close substitutes",
    "change in styles tastes",
    "change in the number of buyers",
    "change in the number of producers",
    "change in the price of substitute goods",
    "changes in technology",
    "changes in the cost of production",
    "solutions to the free rider problem taxes",
    "time",
    "visual logic",
}


def split_csv(value: str) -> list[str]:
    return [part.strip() for part in value.split(",") if part.strip()]


def list_or_csv(value) -> list[str]:
    if isinstance(value, list):
        return [str(part).strip() for part in value if str(part).strip()]
    if isinstance(value, str):
        return split_csv(value)
    return []


def normalize_term(value: str) -> str:
    return " ".join(part for part in "".join(ch.lower() if ch.isalnum() else " " for ch in value).split())


def vocab_title_issue(term: str) -> str:
    normalized = normalize_term(term)
    if normalized in REJECTED_VOCAB_TERMS:
        return "known extraction-noise term survived into final vocabulary"
    if any(ch in term for ch in "?!:;"):
        return "term still contains sentence punctuation"
    if len(normalized.split()) >= 6:
        return "term is still too sentence-like"
    if any(word in normalized.split() for word in {"because", "when", "where", "which", "that"}) and len(normalized.split()) >= 4:
        return "term still reads like a sentence fragment"
    return ""


def load_graph_model_element_map() -> dict[str, set[str]]:
    try:
        text = GRAPH_MODEL_C.read_text(encoding="utf-8")
    except FileNotFoundError:
        return {}

    array_entries: dict[str, set[str]] = {}
    current_array = ""
    array_header = re.compile(r"static const GraphElementEntry (k_[a-z0-9_]+)\[\] = \{")
    entry_line = re.compile(r'\s*\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"')
    map_line = re.compile(r'\s*\{"([^"]+)",\s*(k_[a-z0-9_]+),')

    for line in text.splitlines():
        header_match = array_header.match(line)
        if header_match:
            current_array = header_match.group(1)
            array_entries[current_array] = set()
            continue
        if current_array:
            if line.strip() == "};":
                current_array = ""
                continue
            entry_match = entry_line.match(line)
            if entry_match:
                array_entries[current_array].add(entry_match.group(1))

    graph_elements: dict[str, set[str]] = {}
    for line in text.splitlines():
        map_match = map_line.match(line)
        if not map_match:
            continue
        graph_elements[map_match.group(1)] = set(array_entries.get(map_match.group(2), set()))
    return graph_elements


def load_header_counts() -> dict[str, int]:
    try:
        text = GENERATED_HEADER.read_text(encoding="utf-8")
    except FileNotFoundError:
        return {}
    matches = re.findall(r"#define\s+([A-Z_]+_COUNT)\s+(\d+)", text)
    return {name: int(value) for name, value in matches}


def validate_graph_focus_ids(
    item: dict,
    label: str,
    graph_elements: dict[str, set[str]],
    errors: list[str],
) -> None:
    graph_id = item.get("graph_id", "").strip()
    if not graph_id:
        related_graph_ids = list_or_csv(item.get("related_graph_ids", item.get("related_graph_ids_csv", "")))
        graph_id = related_graph_ids[0] if related_graph_ids else ""
    if not graph_id:
        return

    valid_ids = graph_elements.get(graph_id, set())
    if not valid_ids:
        return

    graph_element_id = item.get("graph_element_id", "").strip()
    if graph_element_id and graph_element_id not in valid_ids:
        errors.append(f"{label} has invalid graph_element_id {graph_element_id} for graph {graph_id}")

    for field in ("related_curve_ids", "related_curve_ids_csv", "related_point_ids", "related_point_ids_csv", "related_region_ids", "related_region_ids_csv"):
        if not item.get(field):
            continue
        for related_id in list_or_csv(item[field]):
            if related_id not in valid_ids:
                errors.append(f"{label} has invalid {field} entry {related_id} for graph {graph_id}")


def validate_related_ids(
    item: dict,
    label: str,
    field: str,
    valid_ids: set[str],
    errors: list[str],
) -> None:
    for related_id in list_or_csv(item.get(field, item.get(f"{field}_csv", ""))):
        if related_id not in valid_ids:
            errors.append(f"{label} has invalid {field} entry {related_id}")


def main() -> None:
    data = json.loads(CONTENT_JSON.read_text(encoding="utf-8"))
    graph_model_elements = load_graph_model_element_map()
    header_counts = load_header_counts()

    units = {item["id"]: item for item in data["units"]}
    topics = {item["id"]: item for item in data["topics"]}
    graphs = {item["id"]: item for item in data["graphs"]}
    graph_titles = {item["title"] for item in data["graphs"]}
    formulas = {item["id"]: item for item in data["formulas"]}
    formula_titles = {item["title"] for item in data["formulas"]}
    terms = {item["term"] for item in data["vocabulary"]}
    concepts = {item["id"]: item for item in data.get("concepts", [])}
    graph_ids = set(graphs)
    formula_ids = set(formulas)
    vocab_ids = {item["id"] for item in data["vocabulary"]}
    concept_ids = set(concepts)
    header_expected = {
        "UNIT_COUNT": len(data["units"]),
        "TOPIC_COUNT": len(data["topics"]),
        "VOCAB_COUNT": len(data["vocabulary"]),
        "GRAPH_COUNT": len(data["graphs"]),
        "FORMULA_COUNT": len(data["formulas"]),
        "CONCEPT_COUNT": len(data.get("concepts", [])),
        "STRUCTURE_COUNT": len(data["structures"]),
        "QUICK_REVIEW_COUNT": len(data["quick_review"]),
        "EXAM_CRAM_COUNT": len(data["exam_cram"]),
        "REFERENCE_COUNT": len(data["reference"]),
        "AUDIT_COUNT": len(data["audit"]),
    }

    errors: list[str] = []
    warnings: list[str] = []
    vocab_precise_graph_count = 0
    concept_precise_graph_count = 0

    duplicate_terms = [term for term, count in Counter(item["term"] for item in data["vocabulary"]).items() if count > 1]
    if duplicate_terms:
        errors.append(f"Duplicate vocabulary terms: {', '.join(sorted(duplicate_terms))}")
    duplicate_normalized = [term for term, count in Counter(normalize_term(item.get("canonical_term", item["term"])) for item in data["vocabulary"]).items() if count > 1]
    if duplicate_normalized:
        errors.append(f"Duplicate normalized vocabulary keys: {', '.join(sorted(duplicate_normalized))}")
    for macro_name, expected_value in header_expected.items():
        actual_value = header_counts.get(macro_name)
        if actual_value != expected_value:
            errors.append(f"Generated header count mismatch for {macro_name}: header={actual_value} payload={expected_value}")

    for topic in data["topics"]:
        if topic["primary_unit_id"] not in units:
            errors.append(f"Topic {topic['id']} has invalid primary_unit_id {topic['primary_unit_id']}")
        for unit_id in list_or_csv(topic.get("unit_ids", topic.get("unit_ids_csv", ""))):
            if unit_id not in units:
                errors.append(f"Topic {topic['id']} has invalid unit membership {unit_id}")
        validate_related_ids(topic, f"Topic {topic['id']}", "related_graph_ids", graph_ids, errors)
        validate_related_ids(topic, f"Topic {topic['id']}", "related_formula_ids", formula_ids, errors)
        validate_related_ids(topic, f"Topic {topic['id']}", "related_vocab_ids", vocab_ids, errors)
        validate_related_ids(topic, f"Topic {topic['id']}", "related_concept_ids", concept_ids, errors)

    for vocab in data["vocabulary"]:
        if vocab["primary_unit_id"] not in units:
            errors.append(f"Vocabulary {vocab['term']} has invalid primary_unit_id {vocab['primary_unit_id']}")
        if vocab["primary_topic_id"] not in topics:
            errors.append(f"Vocabulary {vocab['term']} has invalid primary_topic_id {vocab['primary_topic_id']}")
        if vocab.get("source_type", "") not in VALID_VOCAB_SOURCES:
            errors.append(f"Vocabulary {vocab['term']} has invalid source_type {vocab.get('source_type', '')}")
        if not vocab.get("canonical_term", "").strip():
            errors.append(f"Vocabulary {vocab['term']} is missing canonical_term")
        if not vocab.get("normalized_key", "").strip():
            errors.append(f"Vocabulary {vocab['term']} is missing normalized_key")
        title_issue = vocab_title_issue(vocab["term"])
        if title_issue:
            errors.append(f"Vocabulary {vocab['term']} failed quality filter: {title_issue}")
        try:
            quality_score = int(str(vocab.get("quality_score", "0")))
        except ValueError:
            errors.append(f"Vocabulary {vocab['term']} has non-numeric quality_score {vocab.get('quality_score', '')}")
            quality_score = 0
        if quality_score < 60:
            errors.append(f"Vocabulary {vocab['term']} has weak quality_score {quality_score}")
        for unit_id in list_or_csv(vocab.get("unit_ids", vocab.get("unit_ids_csv", ""))):
            if unit_id not in units:
                errors.append(f"Vocabulary {vocab['term']} has invalid unit membership {unit_id}")
        for topic_id in list_or_csv(vocab.get("topic_ids", vocab.get("topic_ids_csv", ""))):
            if topic_id not in topics:
                errors.append(f"Vocabulary {vocab['term']} has invalid topic membership {topic_id}")
        if vocab["category"] not in VALID_CATEGORIES:
            errors.append(f"Vocabulary {vocab['term']} has invalid category {vocab['category']}")
        if not vocab["short_definition"].strip():
            errors.append(f"Vocabulary {vocab['term']} is missing a short definition")
        if not vocab["long_definition"].strip():
            errors.append(f"Vocabulary {vocab['term']} is missing a full explanation")
        has_graph_path = bool(
            (vocab.get("graph_name", "").strip() or vocab.get("graph_id", "").strip() or vocab.get("related_graph_ids_csv", "").strip())
            and (vocab.get("graph_location_text", "").strip() or vocab.get("graph_where", "").strip())
        )
        has_example_path = bool(vocab.get("real_example", "").strip() and vocab.get("example_graph_effect", "").strip())
        if not has_graph_path and not has_example_path:
            errors.append(f"Vocabulary {vocab['term']} is missing both graph-location and example-based support")
        if vocab.get("graph_location_text", "").strip():
            vocab_precise_graph_count += 1
        if vocab.get("graph_element_id", "").strip() and not (vocab.get("graph_id", "").strip() or vocab.get("related_graph_ids_csv", "").strip()):
            errors.append(f"Vocabulary {vocab['term']} has graph_element_id but no graph link")
        validate_graph_focus_ids(vocab, f"Vocabulary {vocab['term']}", graph_model_elements, errors)
        validate_related_ids(vocab, f"Vocabulary {vocab['term']}", "related_graph_ids", graph_ids, errors)
        validate_related_ids(vocab, f"Vocabulary {vocab['term']}", "related_formula_ids", formula_ids, errors)
        validate_related_ids(vocab, f"Vocabulary {vocab['term']}", "related_vocab_ids", vocab_ids, errors)
        validate_related_ids(vocab, f"Vocabulary {vocab['term']}", "related_concept_ids", concept_ids, errors)
        for title in split_csv(vocab.get("related_graphs", "")):
            if title not in graph_titles:
                warnings.append(f"Vocabulary {vocab['term']} references unknown graph title {title}")
        for title in split_csv(vocab.get("related_formulas", "")):
            if title not in formula_titles:
                warnings.append(f"Vocabulary {vocab['term']} references unknown formula title {title}")

    for graph in data["graphs"]:
        if graph["primary_unit_id"] and graph["primary_unit_id"] not in units:
            errors.append(f"Graph {graph['id']} has invalid primary_unit_id {graph['primary_unit_id']}")
        for unit_id in list_or_csv(graph.get("unit_ids", graph.get("unit_ids_csv", ""))):
            if unit_id not in units:
                errors.append(f"Graph {graph['id']} has invalid unit membership {unit_id}")
        for topic_id in list_or_csv(graph.get("topic_ids", graph.get("topic_ids_csv", ""))):
            if topic_id not in topics:
                errors.append(f"Graph {graph['id']} has invalid topic membership {topic_id}")
        if not graph["title"].strip():
            errors.append(f"Graph {graph['id']} is missing a title")
        if not graph["overview_page"].strip():
            errors.append(f"Graph {graph['id']} is missing its overview page")
        if not graph["related_terms"].strip():
            warnings.append(f"Graph {graph['id']} has no related terms")
        validate_related_ids(graph, f"Graph {graph['id']}", "related_vocab_ids", vocab_ids, errors)
        validate_related_ids(graph, f"Graph {graph['id']}", "related_formula_ids", formula_ids, errors)
        validate_related_ids(graph, f"Graph {graph['id']}", "related_concept_ids", concept_ids, errors)
        for term in split_csv(graph.get("related_terms", "")):
            if term not in terms:
                warnings.append(f"Graph {graph['id']} references unknown related term {term}")
        for title in split_csv(graph.get("related_formulas", "")):
            if title not in formula_titles:
                warnings.append(f"Graph {graph['id']} references unknown related formula title {title}")

    for formula in data["formulas"]:
        if formula["primary_topic_id"] and formula["primary_topic_id"] not in topics:
            errors.append(f"Formula {formula['id']} has invalid primary_topic_id {formula['primary_topic_id']}")
        if formula["primary_unit_id"] and formula["primary_unit_id"] not in units:
            errors.append(f"Formula {formula['id']} has invalid primary_unit_id {formula['primary_unit_id']}")
        for topic_id in list_or_csv(formula.get("topic_ids", formula.get("topic_ids_csv", ""))):
            if topic_id not in topics:
                errors.append(f"Formula {formula['id']} has invalid topic membership {topic_id}")
        for unit_id in list_or_csv(formula.get("unit_ids", formula.get("unit_ids_csv", ""))):
            if unit_id not in units:
                errors.append(f"Formula {formula['id']} has invalid unit membership {unit_id}")
        if formula["helper"] not in VALID_HELPERS:
            errors.append(f"Formula {formula['id']} has invalid helper id {formula['helper']}")
        if not formula["formula"].strip():
            errors.append(f"Formula {formula['id']} is missing its formula text")
        validate_related_ids(formula, f"Formula {formula['id']}", "related_graph_ids", graph_ids, errors)
        validate_related_ids(formula, f"Formula {formula['id']}", "related_vocab_ids", vocab_ids, errors)
        validate_related_ids(formula, f"Formula {formula['id']}", "related_concept_ids", concept_ids, errors)
        for title in split_csv(formula.get("related_graphs", "")):
            if title not in graph_titles:
                warnings.append(f"Formula {formula['id']} references unknown graph title {title}")
        for term in split_csv(formula.get("related_terms", "")):
            if term not in terms:
                warnings.append(f"Formula {formula['id']} references unknown related term {term}")

    for concept in data.get("concepts", []):
        if concept["primary_unit_id"] not in units:
            errors.append(f"Concept {concept['id']} has invalid primary_unit_id {concept['primary_unit_id']}")
        if concept["primary_topic_id"] and concept["primary_topic_id"] not in topics:
            errors.append(f"Concept {concept['id']} has invalid primary_topic_id {concept['primary_topic_id']}")
        for unit_id in list_or_csv(concept.get("unit_ids", concept.get("unit_ids_csv", ""))):
            if unit_id not in units:
                errors.append(f"Concept {concept['id']} has invalid unit membership {unit_id}")
        for topic_id in list_or_csv(concept.get("topic_ids", concept.get("topic_ids_csv", ""))):
            if topic_id not in topics:
                errors.append(f"Concept {concept['id']} has invalid topic membership {topic_id}")
        if not concept["title"].strip():
            errors.append(f"Concept {concept['id']} is missing a title")
        if not concept["short_definition"].strip():
            errors.append(f"Concept {concept['id']} is missing a short definition")
        if not concept["full_explanation"].strip():
            errors.append(f"Concept {concept['id']} is missing a full explanation")
        if concept["graph_id"] and concept["graph_id"] not in graphs:
            errors.append(f"Concept {concept['id']} has invalid graph_id {concept['graph_id']}")
        if concept.get("graph_location_text", "").strip():
            concept_precise_graph_count += 1
        if concept.get("graph_element_id", "").strip() and not (concept.get("graph_id", "").strip() or concept.get("related_graph_ids_csv", "").strip()):
            errors.append(f"Concept {concept['id']} has graph_element_id but no graph link")
        validate_graph_focus_ids(concept, f"Concept {concept['id']}", graph_model_elements, errors)
        validate_related_ids(concept, f"Concept {concept['id']}", "related_graph_ids", graph_ids, errors)
        validate_related_ids(concept, f"Concept {concept['id']}", "related_vocab_ids", vocab_ids, errors)
        validate_related_ids(concept, f"Concept {concept['id']}", "related_formula_ids", formula_ids, errors)
        for term in split_csv(concept.get("related_terms", "")):
            if term not in terms:
                warnings.append(f"Concept {concept['id']} references unknown related term {term}")
        for title in split_csv(concept.get("related_formulas", "")):
            if title not in formula_titles:
                warnings.append(f"Concept {concept['id']} references unknown formula title {title}")

    report_lines = [
        "AP Micro Content Validation Report",
        "",
        f"Units: {len(data['units'])}",
        f"Topics: {len(data['topics'])}",
        f"Vocabulary: {len(data['vocabulary'])}",
        f"Graphs: {len(data['graphs'])}",
        f"Formulas: {len(data['formulas'])}",
        f"Concepts: {len(data.get('concepts', []))}",
        f"Structures: {len(data['structures'])}",
        f"Vocabulary with precise graph metadata: {vocab_precise_graph_count}",
        f"Concepts with precise graph metadata: {concept_precise_graph_count}",
        "",
        f"Errors: {len(errors)}",
        f"Warnings: {len(warnings)}",
        "",
    ]

    if errors:
        report_lines.append("Blocking Issues")
        report_lines.extend(f"- {item}" for item in errors)
        report_lines.append("")

    if warnings:
        report_lines.append("Warnings")
        report_lines.extend(f"- {item}" for item in warnings)
        report_lines.append("")

    if not errors and not warnings:
        report_lines.append("No validation issues found.")

    REPORT_TXT.write_text("\n".join(report_lines), encoding="utf-8")
    print(f"Wrote {REPORT_TXT}")
    print(f"Errors: {len(errors)}  Warnings: {len(warnings)}")

    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
