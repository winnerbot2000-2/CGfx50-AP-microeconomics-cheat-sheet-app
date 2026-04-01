from __future__ import annotations

import json
import math
from pathlib import Path


CALC_ROOT = Path(__file__).resolve().parent.parent
CONTENT_JSON = CALC_ROOT / "generated" / "apmicro_content.json"
REPORT_TXT = CALC_ROOT / "generated" / "smoke_test_report.txt"


def elasticity(q1: float, q2: float, p1: float, p2: float) -> float:
    return ((q2 - q1) / ((q1 + q2) / 2.0)) / ((p2 - p1) / ((p1 + p2) / 2.0))


def expect_close(name: str, actual: float, expected: float, failures: list[str], tolerance: float = 1e-6) -> None:
    if abs(actual - expected) > tolerance:
        failures.append(f"{name}: expected {expected}, got {actual}")


def expect_true(name: str, condition: bool, failures: list[str]) -> None:
    if not condition:
        failures.append(name)


def main() -> None:
    data = json.loads(CONTENT_JSON.read_text(encoding="utf-8"))
    failures: list[str] = []
    graphs_allowing_empty_formula_links = {"ppc", "lorenz", "trade-table"}
    vocab_precise_graph_count = 0
    concept_precise_graph_count = 0

    expect_true("Missing units", len(data["units"]) == 6, failures)
    expect_true("Missing topics", len(data["topics"]) >= 20, failures)
    expect_true("Missing vocabulary", len(data["vocabulary"]) >= 40, failures)
    expect_true("Missing graphs", len(data["graphs"]) >= 18, failures)
    expect_true("Missing formulas", len(data["formulas"]) >= 15, failures)
    expect_true("Missing concepts", len(data.get("concepts", [])) >= 10, failures)
    expect_true("Missing reference sheets", len(data.get("reference", [])) >= 9, failures)

    for unit in data["units"]:
        expect_true(f"Unit {unit['id']} missing body", bool(unit["body"].strip()), failures)

    for graph in data["graphs"]:
        expect_true(f"Graph {graph['id']} missing related terms", bool(graph["related_terms"].strip()), failures)
        if graph["id"] not in graphs_allowing_empty_formula_links:
            expect_true(f"Graph {graph['id']} missing related formulas", bool(graph["related_formulas"].strip()), failures)

    for vocab in data["vocabulary"]:
        expect_true(f"Vocabulary {vocab['term']} missing full explanation", bool(vocab["long_definition"].strip()), failures)
        expect_true(f"Vocabulary {vocab['term']} missing source_type", bool(vocab.get("source_type", "").strip()), failures)
        expect_true(f"Vocabulary {vocab['term']} missing canonical_term", bool(vocab.get("canonical_term", "").strip()), failures)
        has_graph_path = bool(
            (vocab.get("graph_name", "").strip() or vocab.get("graph_id", "").strip() or vocab.get("related_graph_ids_csv", "").strip())
            and (vocab.get("graph_location_text", "").strip() or vocab.get("graph_where", "").strip())
        )
        has_example_path = bool(vocab.get("real_example", "").strip() and vocab.get("example_graph_effect", "").strip())
        expect_true(f"Vocabulary {vocab['term']} missing graph/example support", has_graph_path or has_example_path, failures)
        if vocab.get("graph_location_text", "").strip():
            vocab_precise_graph_count += 1

    for formula in data["formulas"]:
        expect_true(f"Formula {formula['id']} missing formula text", bool(formula["formula"].strip()), failures)
        expect_true(f"Formula {formula['id']} missing primary_topic_id", bool(formula["primary_topic_id"].strip()), failures)

    for concept in data.get("concepts", []):
        graph_optional = "not graph-based" in concept.get("graph_connection", "").lower() or "comparison-based" in concept.get("graph_connection", "").lower()
        if not graph_optional:
            expect_true(f"Concept {concept['id']} missing graph id", bool(concept["graph_id"].strip()), failures)
        expect_true(f"Concept {concept['id']} missing explanation", bool(concept["full_explanation"].strip()), failures)
        expect_true(f"Concept {concept['id']} missing example", bool(concept["real_world_example"].strip()), failures)
        if concept.get("graph_location_text", "").strip():
            concept_precise_graph_count += 1

    expect_true("Precise vocab graph metadata coverage too low", vocab_precise_graph_count >= 35, failures)
    expect_true("Precise concept graph metadata coverage too low", concept_precise_graph_count >= 20, failures)

    expect_close("PED sample", abs(elasticity(100, 80, 10, 12)), 1.2222222222, failures)
    expect_close("PES sample", abs(elasticity(20, 30, 5, 8)), 0.8666666667, failures)
    expect_close("Cross sample", elasticity(10, 14, 5, 7), 1.0, failures)
    expect_close("Income sample", elasticity(10, 8, 100, 120), -1.2222222222, failures)
    expect_close("TR sample", 5 * 20, 100.0, failures)
    expect_close("MR sample", (140 - 100) / (12 - 10), 20.0, failures)
    expect_close("TC sample", 30 + 50, 80.0, failures)
    expect_close("Profit sample", (10 * 5) - (7 * 5), 15.0, failures)
    expect_close("Accounting profit sample", 100 - 60, 40.0, failures)
    expect_close("Economic profit sample", 100 - 60 - 20, 20.0, failures)
    expect_close("AFC sample", 20 / 10, 2.0, failures)
    expect_close("AVC sample", 40 / 10, 4.0, failures)
    expect_close("ATC sample", (20 + 40) / 10, 6.0, failures)
    expect_close("MC sample", (124 - 100) / (13 - 10), 8.0, failures)
    expect_close("Tax revenue sample", 3 * 20, 60.0, failures)
    expect_close("DWL sample", 0.5 * 10 * 4, 20.0, failures)
    expect_close("MRP sample", 5 * 8, 40.0, failures)

    invalid = elasticity(4, 2, 10, 12)
    expect_true("Elasticity should be finite for normal inputs", math.isfinite(invalid), failures)

    lines = [
        "AP Micro Native Add-In Smoke Test Report",
        "",
        f"Failures: {len(failures)}",
        "",
    ]
    if failures:
        lines.extend(f"- {item}" for item in failures)
    else:
        lines.append("All smoke checks passed.")

    REPORT_TXT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {REPORT_TXT}")
    print(f"Failures: {len(failures)}")

    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
