from __future__ import annotations

import json
import os
import re
import sqlite3
import textwrap
import zipfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
from xml.etree import ElementTree as ET

from manual_content import (
    CONCEPT_DEFS,
    FORMULA_DEFS,
    GRAPH_DEFS,
    MANUAL_TOPIC_BUNDLES,
    MANDATORY_VOCAB,
    QUICK_REVIEW,
    REFERENCE_SHEETS,
    EXAM_CRAM,
    MARKET_STRUCTURES,
    TERM_CATEGORY_OVERRIDES,
    TERM_CONFUSIONS,
    TERM_EXAMPLE_DETAILS,
    TERM_GRAPH_DETAILS,
    TERM_GRAPH_LOCATION_DETAILS,
    TERM_RELATED,
    TOPIC_KEY_IDEAS,
    UNIT_MISTAKES,
    UNIT_OVERVIEW_NOTES,
    UNIT_REMINDERS,
    UNIT_TITLES,
    TOPIC_OVERVIEWS,
    CONCEPT_GRAPH_LOCATION_DETAILS,
)


SCRIPT_DIR = Path(__file__).resolve().parent
CALC_ROOT = SCRIPT_DIR.parent
DEFAULT_SOURCE_ROOT = CALC_ROOT.parent
SOURCE_ROOT_PLACEHOLDER = "<external-source-root>"
OUTPUT_JSON = CALC_ROOT / "generated" / "apmicro_content.json"
OUTPUT_C = CALC_ROOT / "src" / "generated" / "apmicro_content.c"
OUTPUT_H = CALC_ROOT / "src" / "generated" / "apmicro_content.h"
GRAPH_MODEL_C = CALC_ROOT / "src" / "graph_model.c"


TOPIC_GRAPH_MAP = {
    "scarcity-choice": ["ppc"],
    "opportunity-cost": ["ppc"],
    "ppc": ["ppc"],
    "comparative-advantage": ["ppc", "trade-table"],
    "specialization-trade": ["trade-table", "ppc"],
    "economic-systems": [],
    "equilibrium": ["supply-demand", "surplus-welfare"],
    "demand-supply-shifts": ["supply-demand"],
    "elasticity": ["supply-demand"],
    "taxes-and-subsidies": ["tax", "subsidy"],
    "price-controls": ["price-ceiling", "price-floor"],
    "trade-policy": ["tax"],
    "cost-curves": ["cost-curves", "production-function"],
    "production-functions": ["production-function", "cost-curves", "ppc"],
    "perfect-competition": ["perfect-competition", "perfect-competition-market"],
    "monopoly": ["monopoly"],
    "monopolistic-competition-oligopoly": ["monopolistic-competition", "monopoly"],
    "monopolistic-competition": ["monopolistic-competition"],
    "oligopoly": ["kinked-demand", "game-theory-matrix"],
    "collusion": ["monopoly", "game-theory-matrix"],
    "game-theory": ["game-theory-matrix"],
    "barriers-to-entry": ["monopoly"],
    "price-discrimination": ["monopoly"],
    "imperfect-profit-max": ["monopoly", "monopolistic-competition"],
    "imperfect-efficiency": ["monopoly", "monopolistic-competition"],
    "factor-markets-labor": ["labor-market", "hiring-rule", "monopsony"],
    "factor-markets": ["labor-market", "hiring-rule", "monopsony"],
    "derived-demand": ["labor-market", "hiring-rule"],
    "mrp-mrc-hiring": ["hiring-rule", "monopsony"],
    "competitive-labor-market": ["labor-market", "hiring-rule"],
    "wage-determination": ["labor-market", "monopsony"],
    "labor-market-equilibrium": ["labor-market"],
    "resource-demand-shifters": ["labor-market", "hiring-rule"],
    "resource-supply-shifters": ["labor-market"],
    "externalities": ["negative-externality", "positive-externality"],
    "negative-externalities": ["negative-externality"],
    "positive-externalities": ["positive-externality"],
    "government-corrective-policy": ["negative-externality", "positive-externality", "tax", "subsidy", "public-goods-common-resources"],
    "public-common-goods": ["public-goods-common-resources", "negative-externality"],
    "public-goods": ["public-goods-common-resources"],
    "common-resources": ["public-goods-common-resources", "negative-externality"],
    "free-rider-problem": ["public-goods-common-resources"],
    "inequality": ["lorenz"],
    "market-failure": ["negative-externality", "positive-externality", "public-goods-common-resources", "lorenz"],
}

TOPIC_FORMULA_MAP = {
    "scarcity-choice": [],
    "opportunity-cost": [],
    "ppc": [],
    "comparative-advantage": [],
    "specialization-trade": [],
    "economic-systems": [],
    "equilibrium": ["total-revenue", "deadweight-loss"],
    "demand-supply-shifts": ["ped", "pes", "cross-elasticity", "income-elasticity", "midpoint-elasticity"],
    "elasticity": ["ped", "pes", "cross-elasticity", "income-elasticity", "midpoint-elasticity", "total-revenue"],
    "taxes-and-subsidies": ["tax-revenue", "deadweight-loss"],
    "price-controls": ["deadweight-loss"],
    "trade-policy": ["deadweight-loss"],
    "cost-curves": ["total-cost", "average-costs", "marginal-cost", "profit", "accounting-economic-profit"],
    "production-functions": ["marginal-cost"],
    "perfect-competition": ["total-cost", "average-costs", "marginal-cost", "profit", "marginal-revenue-basic"],
    "monopoly": ["profit", "deadweight-loss", "total-revenue", "marginal-revenue-basic"],
    "monopolistic-competition-oligopoly": ["profit"],
    "monopolistic-competition": ["profit", "marginal-revenue-basic", "imperfect-profit-max", "imperfect-efficiency"],
    "oligopoly": ["marginal-revenue-basic", "total-revenue", "imperfect-profit-max"],
    "collusion": ["total-revenue", "imperfect-profit-max"],
    "game-theory": ["total-revenue"],
    "barriers-to-entry": ["imperfect-efficiency"],
    "price-discrimination": ["total-revenue", "imperfect-profit-max"],
    "imperfect-profit-max": ["marginal-revenue-basic", "profit", "imperfect-profit-max"],
    "imperfect-efficiency": ["deadweight-loss", "imperfect-efficiency"],
    "factor-markets-labor": ["mrp", "labor-hiring-rule"],
    "factor-markets": ["mrp", "labor-hiring-rule"],
    "derived-demand": ["mrp"],
    "mrp-mrc-hiring": ["mrp", "labor-hiring-rule"],
    "competitive-labor-market": ["mrp", "labor-hiring-rule"],
    "wage-determination": ["labor-hiring-rule"],
    "labor-market-equilibrium": ["labor-hiring-rule"],
    "resource-demand-shifters": ["mrp"],
    "resource-supply-shifters": ["labor-hiring-rule"],
    "externalities": ["deadweight-loss"],
    "negative-externalities": ["deadweight-loss"],
    "positive-externalities": ["deadweight-loss"],
    "government-corrective-policy": ["deadweight-loss", "tax-revenue"],
    "public-common-goods": [],
    "public-goods": [],
    "common-resources": ["deadweight-loss"],
    "free-rider-problem": [],
    "inequality": [],
    "market-failure": ["deadweight-loss"],
}

GRAPH_FORMULA_OVERRIDES = {
    "supply-demand": "Price Elasticity of Demand, Price Elasticity of Supply, Cross-Price Elasticity, Income Elasticity, Midpoint Elasticity, Total Revenue, Deadweight Loss",
    "ppc": "",
    "trade-table": "",
    "tax": "Tax Revenue, Deadweight Loss",
    "subsidy": "Tax Revenue, Deadweight Loss",
    "price-ceiling": "Deadweight Loss",
    "price-floor": "Deadweight Loss",
    "cost-curves": "Total Cost, Economic Profit, Accounting vs Economic Profit, Average Cost Family, Marginal Cost",
    "perfect-competition": "Marginal Revenue Basics, Total Cost, Economic Profit, Average Cost Family, Marginal Cost",
    "perfect-competition-market": "Marginal Revenue Basics",
    "monopoly": "Total Revenue, Marginal Revenue Basics, Economic Profit, Deadweight Loss",
    "monopolistic-competition": "Economic Profit, Marginal Revenue Basics",
    "kinked-demand": "Marginal Revenue Basics, Total Revenue, Imperfect Competition Profit Rule",
    "game-theory-matrix": "Total Revenue",
    "monopsony": "Marginal Revenue Product, Hiring Rule for Labor",
    "production-function": "Marginal Cost",
    "labor-market": "Marginal Revenue Product, Hiring Rule for Labor",
    "hiring-rule": "Marginal Revenue Product, Hiring Rule for Labor",
    "surplus-welfare": "Total Revenue, Deadweight Loss",
    "negative-externality": "Deadweight Loss",
    "positive-externality": "Deadweight Loss",
    "public-goods-common-resources": "Deadweight Loss",
    "lorenz": "",
}

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

VOCAB_CANONICAL_TERM_OVERRIDES = {
    "cross price elasticity": "Cross-Price Elasticity",
}

VOCAB_REJECTED_EXACT_TERMS = {
    "availability of close substitutes": "determinant fragment instead of a clean vocab term",
    "change in styles tastes": "determinant fragment instead of a clean vocab term",
    "change in the number of buyers": "determinant fragment instead of a clean vocab term",
    "change in the number of producers": "determinant fragment instead of a clean vocab term",
    "change in the price of substitute goods": "determinant fragment instead of a clean vocab term",
    "changes in technology": "determinant fragment instead of a clean vocab term",
    "changes in the cost of production": "determinant fragment instead of a clean vocab term",
    "solutions to the free rider problem taxes": "sentence-like heading fragment",
    "time": "too broad to be a trustworthy AP Micro vocab entry",
    "visual logic": "note fragment instead of a vocab term",
}

VOCAB_REJECTED_PREFIXES = {
    "solutions to ": "sentence-like heading fragment",
}

VOCAB_APPROVED_SHORT_EXTRACTED = {
    "market equilibrium",
    "shortage",
    "surplus",
    "constant returns to scale",
    "cross price elasticity",
}

VOCAB_EXTRACTED_DEFINITION_TAILS = [
    "Reasons for a change in Demand",
    "Reasons for a change in Supply",
    "Determinants of the Price Elasticity of Demand",
    "Long-Run Costs LATC Cost Quantity",
]


@dataclass
class ChunkRecord:
    id: str
    heading: str
    text: str
    source_id: str
    topic_slug: str
    unit_id: str


def sanitize_text(value: str) -> str:
    value = value.replace("\u2019", "'").replace("\u2018", "'").replace("\u201c", '"').replace("\u201d", '"')
    value = value.replace("\u2013", "-").replace("\u2014", "-").replace("\u2011", "-")
    value = value.replace("\u2022", "-").replace("\xa0", " ")
    value = re.sub(r"\s+", " ", value.replace("|", " | ")).strip()
    value = re.sub(r"\s*\|\s*", "\n", value)
    value = re.sub(r"\n{3,}", "\n\n", value)
    return value.strip()


def short_title_from_path(path: Path) -> str:
    label = re.sub(r"[_-]+", " ", path.stem).strip()
    return re.sub(r"\s{2,}", " ", label)


def resolve_source_root() -> Path:
    configured = os.environ.get("APMICRO_SOURCE_ROOT", "").strip()
    if configured:
        return Path(configured).expanduser().resolve()
    return DEFAULT_SOURCE_ROOT


def find_best_db(source_root: Path) -> Path:
    candidates = [
        source_root / "apps" / "api" / ".data" / "apmicro-lemon.db",
        source_root / "apps" / "api" / ".data" / "apmicro-mixed.db",
        source_root / "apps" / "api" / "apmicro.db",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("No AP Microeconomics SQLite source database found.")


def parse_slide_xml(xml_bytes: bytes) -> list[str]:
    root = ET.fromstring(xml_bytes)
    texts: list[str] = []
    for node in root.iter():
        tag = node.tag.rsplit("}", 1)[-1]
        if tag in {"t", "a:t"} and node.text:
            cleaned = sanitize_text(node.text)
            if cleaned:
                texts.append(cleaned)
    return texts


def extract_office_text(path: Path) -> list[str]:
    try:
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            if path.suffix.lower() == ".pptx":
                slide_names = sorted(name for name in names if name.startswith("ppt/slides/slide") and name.endswith(".xml"))
                lines: list[str] = []
                for slide_name in slide_names:
                    slide_text = parse_slide_xml(archive.read(slide_name))
                    if slide_text:
                        lines.append("\n".join(slide_text))
                return lines
            if path.suffix.lower() == ".docx":
                if "word/document.xml" not in names:
                    return []
                return ["\n".join(parse_slide_xml(archive.read("word/document.xml")))]
    except zipfile.BadZipFile:
        return []
    return []


def public_source_path(path: Path, source_root: Path) -> str:
    try:
        return path.resolve().relative_to(source_root.resolve()).as_posix()
    except ValueError:
        return path.name


def scan_repo_sources(source_root: Path) -> tuple[dict, dict]:
    lemon_dir = source_root / "lemon-microeconomics"
    pptx = sorted(lemon_dir.rglob("*.pptx"))
    docx = sorted(lemon_dir.rglob("*.docx"))
    graph_modules = sorted((source_root / "apps" / "web" / "components" / "graphs").glob("*-module.tsx"))
    db_path = find_best_db(source_root)
    report = {
        "repo_root": SOURCE_ROOT_PLACEHOLDER,
        "database": public_source_path(db_path, source_root),
        "powerpoints": [public_source_path(path, source_root) for path in pptx],
        "documents": [public_source_path(path, source_root) for path in docx],
        "graph_modules": [public_source_path(path, source_root) for path in graph_modules],
    }
    source_paths = {
        "database": db_path,
        "powerpoints": pptx,
        "documents": docx,
        "graph_modules": graph_modules,
    }
    return report, source_paths


def load_db_topics(db_path: Path) -> tuple[list[dict], list[ChunkRecord]]:
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    topics: list[dict] = []
    topic_by_chunk: dict[str, tuple[str, str]] = {}
    for row in cur.execute("SELECT topic_slug, title, unit_id, summary, chunk_ids FROM topic_bundles ORDER BY unit_id, title"):
        chunk_ids = json.loads(row["chunk_ids"]) if row["chunk_ids"] else []
        topics.append(
            {
                "topic_slug": row["topic_slug"],
                "title": sanitize_text(row["title"]),
                "unit_id": row["unit_id"] or "unit-0",
                "summary": sanitize_text(row["summary"]),
                "chunk_ids": chunk_ids,
            }
        )
        for chunk_id in chunk_ids:
            topic_by_chunk[chunk_id] = (row["topic_slug"], row["unit_id"] or "unit-0")

    chunks: list[ChunkRecord] = []
    for row in cur.execute("SELECT id, heading, text, source_id FROM chunks ORDER BY id"):
        topic_info = topic_by_chunk.get(row["id"])
        if not topic_info:
            continue
        topic_slug, unit_id = topic_info
        chunks.append(
            ChunkRecord(
                id=row["id"],
                heading=sanitize_text(row["heading"]),
                text=sanitize_text(row["text"]),
                source_id=row["source_id"],
                topic_slug=topic_slug,
                unit_id=unit_id,
            )
        )
    conn.close()
    return topics, chunks


def split_candidate_lines(text: str) -> list[str]:
    parts = []
    for piece in re.split(r"[\n\r]+", text):
        cleaned = sanitize_text(piece)
        if not cleaned:
            continue
        parts.append(cleaned)
    return parts


def looks_like_noise(line: str) -> bool:
    lower = line.lower()
    if lower.startswith("slide ") or lower == "ap microeconomics":
        return True
    if lower.startswith("http://") or lower.startswith("https://"):
        return True
    if re.fullmatch(r"[\d$%.,: ]+", line):
        return True
    if len(re.findall(r"[A-Za-z]", line)) < 4:
        return True
    if line.count(" | ") >= 3:
        return True
    return False


def normalize_term(term: str) -> str:
    lowered = re.sub(r"[^a-z0-9]+", " ", term.lower()).strip()
    return re.sub(r"\s+", " ", lowered)


def good_term(term: str) -> bool:
    lowered = term.lower()
    if len(term) < 3 or len(term) > 48:
        return False
    if term.endswith("?"):
        return False
    if lowered.startswith(("what ", "why ", "how ", "which ", "example", "examples")):
        return False
    banned_fragments = [
        "various answers",
        "before the tax",
        "graph design",
        "definition",
        "or what else",
        "will suffice",
        "price discrimination price discrimination",
        "game theory game theory",
        "costs of production marginal cost",
        "monopoly demand monopolists",
        "price floors and price ceilings price floor",
        "other key factors",
    ]
    if any(fragment in lowered for fragment in banned_fragments):
        return False
    if len(re.findall(r"[A-Za-z]", term)) < 3:
        return False
    if sum(1 for ch in term if ch.isdigit()) > 5:
        return False
    word_count = len(term.split())
    return 1 <= word_count <= 8


def good_definition(text: str) -> bool:
    if len(text) < 18 or len(text) > 260:
        return False
    if len(re.findall(r"[A-Za-z]", text)) < 12:
        return False
    if text.endswith("?"):
        return False
    if text.count("$") > 3 or text.count("%") > 4:
        return False
    return True


def maybe_definition_from_pair(first: str, second: str) -> tuple[str, str] | None:
    term = first.strip(" :-")
    definition = second.strip()
    if good_term(term) and good_definition(definition):
        return term, definition
    return None


def normalize_display_term(term: str) -> str:
    cleaned = sanitize_text(term).strip(" :-,;.")
    normalized = normalize_term(cleaned)
    override = VOCAB_CANONICAL_TERM_OVERRIDES.get(normalized)
    if override:
        return override
    if normalized in {"mpc", "msc", "mpb", "msb", "mr", "mc", "tr", "tc", "atc", "avc", "afc", "mrp", "mrc"}:
        return normalized.upper()
    if cleaned.isupper() and len(cleaned) <= 8:
        return cleaned
    return cleaned


def normalize_extracted_definition(text: str) -> str:
    cleaned = sanitize_text(text).strip(" :-")
    cleaned = re.sub(r"\bExample\s*-\s*", "Example: ", cleaned, flags=re.IGNORECASE)
    cleaned = re.sub(r"\s{2,}", " ", cleaned)
    for marker in VOCAB_EXTRACTED_DEFINITION_TAILS:
        if marker.lower() in cleaned.lower():
            cleaned = re.split(re.escape(marker), cleaned, flags=re.IGNORECASE)[0].strip(" -:;,.")
    sentences = re.split(r"(?<=[.!?])\s+", cleaned)
    if len(cleaned) > 190 and len(sentences) > 1:
        cleaned = " ".join(sentences[:2]).strip()
    if len(cleaned) > 220:
        cleaned = cleaned[:220].rsplit(" ", 1)[0].rstrip(" ,;:-") + "."
    return cleaned


def clean_vocab_heading(term: str, heading: str) -> str:
    cleaned = sanitize_text(heading).strip(" :-")
    if not cleaned:
        return ""
    if normalize_term(cleaned) == normalize_term(term):
        return ""
    if len(cleaned) > 42:
        return ""
    if looks_like_noise(cleaned):
        return ""
    if re.search(r"\b(reasons?|determinants?|solutions?)\b", cleaned.lower()):
        return ""
    return cleaned


def vocab_candidate_rejection_reason(term: str, definition: str) -> str:
    normalized = normalize_term(term)
    if normalized in VOCAB_REJECTED_EXACT_TERMS:
        return VOCAB_REJECTED_EXACT_TERMS[normalized]
    for prefix, reason in VOCAB_REJECTED_PREFIXES.items():
        if normalized.startswith(prefix):
            return reason
    if re.search(r"[?!]", term):
        return "sentence-like or malformed title"
    if re.search(r"\b(reasons?|determinants?)\b", normalized):
        return "heading fragment instead of a vocab term"
    if re.search(r"\b(and|because|when|where|which|that)\b", normalized) and len(normalized.split()) >= 5:
        return "sentence-like phrase instead of a vocab term"
    if len(normalized.split()) >= 6 and normalized not in VOCAB_APPROVED_SHORT_EXTRACTED:
        return "overly long phrase instead of a concise vocab term"
    if len(normalized.split()) == 1 and normalized not in VOCAB_APPROVED_SHORT_EXTRACTED and normalized not in {normalize_term(item["term"]) for item in MANDATORY_VOCAB}:
        generic_one_word = {"time", "market", "price", "cost", "benefit", "quantity"}
        if normalized in generic_one_word:
            return "too broad to be a trustworthy AP Micro vocab entry"
    if not good_definition(definition):
        return "definition quality was too weak after normalization"
    return ""


def vocab_candidate_quality_score(term: str, definition: str, heading: str) -> int:
    score = 100
    word_count = len(term.split())
    if word_count >= 5:
        score -= 12
    if word_count == 1 and normalize_term(term) not in VOCAB_APPROVED_SHORT_EXTRACTED:
        score -= 10
    if re.search(r"\d", definition):
        score -= 10
    if re.search(r"\b(example|for example)\b", definition.lower()):
        score -= 4
    if re.search(r"\b(reasons?|determinants?)\b", definition.lower()):
        score -= 12
    if len(definition) > 180:
        score -= 6
    if clean_vocab_heading(term, heading):
        score += 2
    return max(0, min(100, score))


def extract_source_vocab_raw(chunks: list[ChunkRecord]) -> list[dict]:
    records: list[dict] = []
    seen: set[tuple[str, str, str]] = set()
    grouped: dict[str, list[ChunkRecord]] = defaultdict(list)
    for chunk in chunks:
        grouped[chunk.topic_slug].append(chunk)

    for topic_slug, topic_chunks in grouped.items():
        for chunk in topic_chunks:
            lines = [line for line in split_candidate_lines(chunk.text) if not looks_like_noise(line)]
            for index, line in enumerate(lines):
                match = re.match(r"^([^:.-][A-Za-z][A-Za-z0-9 /(),'%+-]{1,48})\s*[:\-]\s+(.+)$", line)
                candidate: tuple[str, str] | None = None
                if match:
                    candidate = (match.group(1).strip(), match.group(2).strip())
                elif line.endswith(":") and index + 1 < len(lines):
                    candidate = maybe_definition_from_pair(line[:-1], lines[index + 1])
                elif index + 1 < len(lines):
                    next_line = lines[index + 1]
                    if good_term(line) and re.match(r"^(is|are|means|refers|shows|states|describes|measures|the )\b", next_line.lower()):
                        candidate = (line, next_line)
                    elif good_term(line) and next_line.lower().startswith(("the ", "a ", "an ")):
                        candidate = (line, next_line)

                if not candidate:
                    continue
                term, definition = candidate
                term = re.sub(r"\s{2,}", " ", term).strip()
                definition = re.sub(r"\s{2,}", " ", definition).strip()
                if not (good_term(term) and good_definition(definition)):
                    continue
                dedupe_key = (normalize_term(term), normalize_term(definition), chunk.topic_slug)
                if dedupe_key in seen:
                    continue
                seen.add(dedupe_key)
                records.append(
                    {
                        "raw_term": term,
                        "raw_definition": definition,
                        "heading": chunk.heading,
                        "source_id": chunk.source_id,
                        "primary_unit_id": chunk.unit_id,
                        "unit_ids": [chunk.unit_id],
                        "primary_topic_id": chunk.topic_slug,
                        "topic_ids": [chunk.topic_slug],
                        "related": unique_csv(TOPIC_GRAPH_MAP.get(chunk.topic_slug, []) + TOPIC_FORMULA_MAP.get(chunk.topic_slug, [])),
                        "source_type": "extracted",
                    }
                )
    return records


def normalize_vocab_candidates(source_vocab_raw: list[dict]) -> list[dict]:
    candidates: list[dict] = []
    for raw in source_vocab_raw:
        display_term = normalize_display_term(raw["raw_term"])
        canonical_term = normalize_display_term(display_term)
        definition = normalize_extracted_definition(raw["raw_definition"])
        normalized_key = normalize_term(canonical_term)
        aliases = []
        raw_display = normalize_display_term(raw["raw_term"])
        if raw_display != canonical_term:
            aliases.append(raw_display)
        heading = clean_vocab_heading(canonical_term, raw.get("heading", ""))
        rejection_reason = vocab_candidate_rejection_reason(canonical_term, definition)
        quality_score = vocab_candidate_quality_score(canonical_term, definition, raw.get("heading", ""))
        candidates.append(
            {
                "raw_term": raw["raw_term"],
                "term": canonical_term,
                "canonical_term": canonical_term,
                "normalized_key": normalized_key,
                "definition": definition,
                "extra": f"AP cue: {heading}." if heading else "",
                "aliases": unique_list(aliases),
                "primary_unit_id": raw["primary_unit_id"],
                "unit_ids": raw.get("unit_ids", [raw["primary_unit_id"]]),
                "primary_topic_id": raw["primary_topic_id"],
                "topic_ids": raw.get("topic_ids", [raw["primary_topic_id"]]),
                "related": raw.get("related", ""),
                "source_type": "extracted",
                "source_id": raw.get("source_id", ""),
                "source_heading": raw.get("heading", ""),
                "quality_score": quality_score,
                "approval_status": "rejected" if rejection_reason else "approved",
                "rejection_reason": rejection_reason,
            }
        )
    return candidates


def choose_better_candidate(existing: dict, candidate: dict) -> dict:
    existing_rank = (existing.get("quality_score", 0), len(existing.get("definition", "")))
    candidate_rank = (candidate.get("quality_score", 0), len(candidate.get("definition", "")))
    return candidate if candidate_rank > existing_rank else existing


def approve_vocab_candidates(normalized_vocab_candidates: list[dict]) -> list[dict]:
    approved: dict[str, dict] = {}
    for candidate in normalized_vocab_candidates:
        if candidate.get("rejection_reason"):
            continue
        key = candidate["normalized_key"]
        existing = approved.get(key)
        if existing is None:
            approved[key] = {
                "term": candidate["term"],
                "canonical_term": candidate["canonical_term"],
                "normalized_key": key,
                "definition": candidate["definition"],
                "extra": candidate.get("extra", ""),
                "aliases": candidate.get("aliases", []),
                "primary_unit_id": candidate["primary_unit_id"],
                "unit_ids": candidate.get("unit_ids", [candidate["primary_unit_id"]]),
                "primary_topic_id": candidate["primary_topic_id"],
                "topic_ids": candidate.get("topic_ids", [candidate["primary_topic_id"]]),
                "related": candidate.get("related", ""),
                "source_type": "extracted",
                "quality_score": candidate.get("quality_score", 0),
                "source_ids": unique_list([candidate.get("source_id", "")]),
                "source_headings": unique_list([candidate.get("source_heading", "")]),
            }
            continue
        best = choose_better_candidate(existing, candidate)
        existing["term"] = best["term"]
        existing["canonical_term"] = best["canonical_term"]
        existing["definition"] = best["definition"]
        existing["extra"] = best.get("extra", existing.get("extra", ""))
        existing["aliases"] = unique_list(existing.get("aliases", []) + candidate.get("aliases", []) + [existing["term"], candidate["term"]])
        existing["aliases"] = [alias for alias in existing["aliases"] if alias != existing["canonical_term"]]
        existing["unit_ids"] = unique_list(existing.get("unit_ids", []) + candidate.get("unit_ids", []))
        existing["topic_ids"] = unique_list(existing.get("topic_ids", []) + candidate.get("topic_ids", []))
        existing["related"] = unique_csv(split_csv_titles(existing.get("related", "")) + split_csv_titles(candidate.get("related", "")))
        existing["quality_score"] = max(existing.get("quality_score", 0), candidate.get("quality_score", 0))
        existing["source_ids"] = unique_list(existing.get("source_ids", []) + [candidate.get("source_id", "")])
        existing["source_headings"] = unique_list(existing.get("source_headings", []) + [candidate.get("source_heading", "")])
    return sorted(approved.values(), key=lambda item: item["term"].lower())


def merge_manual_and_extracted_vocab(approved_vocab: list[dict]) -> list[dict]:
    merged: dict[str, dict] = {}
    for item in MANDATORY_VOCAB:
        normalized_key = normalize_term(item["term"])
        merged[normalized_key] = {
            "term": item["term"],
            "canonical_term": item["term"],
            "normalized_key": normalized_key,
            "definition": item["definition"],
            "extra": item.get("extra", ""),
            "aliases": [],
            "primary_unit_id": item["unit_id"],
            "unit_ids": unique_list([item["unit_id"]]),
            "primary_topic_id": item["topic_slug"],
            "topic_ids": unique_list([item["topic_slug"]]),
            "related": item.get("related", ""),
            "source_type": "manual",
            "quality_score": 100,
        }

    for item in approved_vocab:
        key = item["normalized_key"]
        existing = merged.get(key)
        if existing is None:
            merged[key] = {
                "term": item["term"],
                "canonical_term": item["canonical_term"],
                "normalized_key": key,
                "definition": item["definition"],
                "extra": item.get("extra", ""),
                "aliases": unique_list(item.get("aliases", [])),
                "primary_unit_id": item["primary_unit_id"],
                "unit_ids": unique_list(item.get("unit_ids", [item["primary_unit_id"]])),
                "primary_topic_id": item["primary_topic_id"],
                "topic_ids": unique_list(item.get("topic_ids", [item["primary_topic_id"]])),
                "related": item.get("related", ""),
                "source_type": "extracted",
                "quality_score": item.get("quality_score", 0),
            }
            continue
        existing["source_type"] = "merged"
        existing["unit_ids"] = unique_list(existing.get("unit_ids", []) + item.get("unit_ids", []))
        existing["topic_ids"] = unique_list(existing.get("topic_ids", []) + item.get("topic_ids", []))
        existing["aliases"] = unique_list(existing.get("aliases", []) + item.get("aliases", []) + [item["term"]])
        existing["aliases"] = [alias for alias in existing["aliases"] if alias != existing["canonical_term"]]
        existing["related"] = unique_csv(split_csv_titles(existing.get("related", "")) + split_csv_titles(item.get("related", "")))
        existing["quality_score"] = max(existing.get("quality_score", 0), item.get("quality_score", 0))
        if not existing.get("extra") and item.get("extra"):
            existing["extra"] = item["extra"]
    return sorted(merged.values(), key=lambda item: item["term"].lower())


def build_vocab_pipeline(chunks: list[ChunkRecord]) -> dict[str, object]:
    source_vocab_raw = extract_source_vocab_raw(chunks)
    normalized_vocab_candidates = normalize_vocab_candidates(source_vocab_raw)
    approved_vocab = approve_vocab_candidates(normalized_vocab_candidates)
    merged_vocab_output = merge_manual_and_extracted_vocab(approved_vocab)
    rejected = [item for item in normalized_vocab_candidates if item.get("rejection_reason")]
    renamed = [
        {
            "from": item.get("raw_term", item["canonical_term"]),
            "to": item["canonical_term"],
        }
        for item in normalized_vocab_candidates
        if normalize_term(item.get("raw_term", "")) != item["normalized_key"]
    ]
    merged_aliases = [
        {
            "canonical_term": item["canonical_term"],
            "source_type": item["source_type"],
            "aliases": item.get("aliases", []),
            "topic_ids": item.get("topic_ids", []),
        }
        for item in merged_vocab_output
        if item.get("aliases") or item.get("source_type") != "manual"
    ]
    return {
        "source_vocab_raw": source_vocab_raw,
        "normalized_vocab_candidates": normalized_vocab_candidates,
        "approved_vocab": approved_vocab,
        "merged_vocab_output": merged_vocab_output,
        "report": {
            "source_vocab_raw_count": len(source_vocab_raw),
            "normalized_candidate_count": len(normalized_vocab_candidates),
            "approved_extracted_count": len(approved_vocab),
            "manual_core_count": len(MANDATORY_VOCAB),
            "merged_vocab_count": len(merged_vocab_output),
            "rejection_summary": dict(sorted(Counter(item["rejection_reason"] for item in rejected).items())),
            "rejected_candidates": [
                {
                    "term": item["canonical_term"],
                    "topic_id": item["primary_topic_id"],
                    "reason": item["rejection_reason"],
                }
                for item in sorted(rejected, key=lambda entry: (entry["canonical_term"].lower(), entry["primary_topic_id"]))
            ],
            "approved_extracted_terms": [item["canonical_term"] for item in approved_vocab],
            "merged_aliases": merged_aliases,
            "renamed_candidates": renamed,
        },
    }


def extract_vocab_from_chunks(chunks: list[ChunkRecord]) -> list[dict]:
    return build_vocab_pipeline(chunks)["merged_vocab_output"]


def summary_is_noisy(summary: str) -> bool:
    if len(summary) < 80:
        return True
    digits = sum(1 for ch in summary if ch.isdigit())
    punctuation = sum(1 for ch in summary if ch in "$%=:")
    if digits > 18 or punctuation > 18:
        return True
    return False


def select_topic_key_ideas(topic_slug: str, chunks: Iterable[ChunkRecord]) -> list[str]:
    ideas: list[str] = []
    seen: set[str] = set()
    for idea in TOPIC_KEY_IDEAS.get(topic_slug, []):
        cleaned = sanitize_text(idea)
        normalized = cleaned.lower()
        if cleaned and normalized not in seen:
            ideas.append(cleaned)
            seen.add(normalized)
    for chunk in chunks:
        for line in split_candidate_lines(chunk.text):
            cleaned = sanitize_text(line)
            normalized = cleaned.lower()
            if normalized in seen or looks_like_noise(cleaned):
                continue
            if len(cleaned) < 35 or len(cleaned) > 190:
                continue
            if cleaned.count("-") > 4:
                continue
            if re.search(r"\b(point|slide|ap microeconomics)\b", normalized):
                continue
            ideas.append(cleaned)
            seen.add(normalized)
            if len(ideas) >= 7:
                return ideas
    return ideas


def merge_manual_topics(topics: list[dict]) -> list[dict]:
    by_unit: dict[str, list[dict]] = defaultdict(list)
    seen = {topic["topic_slug"] for topic in topics}
    for topic in MANUAL_TOPIC_BUNDLES:
        if topic["topic_slug"] not in seen:
            by_unit[topic["unit_id"]].append(dict(topic))

    merged: list[dict] = []
    inserted: set[str] = set()
    for topic in topics:
        merged.append(topic)
        unit_id = topic["unit_id"]
        if unit_id in by_unit and unit_id not in inserted:
            merged.extend(by_unit[unit_id])
            inserted.add(unit_id)

    for unit_id, extra_topics in by_unit.items():
        if unit_id not in inserted:
            merged.extend(extra_topics)

    return merged


def build_unit_records(topics: list[dict], vocab: list[dict]) -> list[dict]:
    unit_topics: dict[str, list[dict]] = defaultdict(list)
    for topic in topics:
        unit_topics[topic["unit_id"]].append(topic)

    unit_vocab: dict[str, list[str]] = defaultdict(list)
    for entry in vocab:
        for unit_id in entry.get("unit_ids", [entry.get("primary_unit_id", "")]):
            if unit_id and len(unit_vocab[unit_id]) < 14:
                unit_vocab[unit_id].append(entry["term"])

    records = []
    for unit_id, title in UNIT_TITLES.items():
        topic_titles = [topic["title"] for topic in unit_topics.get(unit_id, [])]
        body_parts = [
            f"Overview\n{title}",
            "Core ideas\n- " + "\n- ".join(UNIT_OVERVIEW_NOTES.get(unit_id, [])),
            "Topics\n- " + "\n- ".join(topic_titles or ["No topics extracted"]),
            "Key vocab\n- " + "\n- ".join(unit_vocab.get(unit_id, []) or ["See vocabulary section"]),
            "Common mistakes\n- " + "\n- ".join(UNIT_MISTAKES.get(unit_id, [])),
            "Exam reminders\n- " + "\n- ".join(UNIT_REMINDERS.get(unit_id, [])),
        ]
        records.append(
            {
                "id": unit_id,
                "title": f"Unit {unit_id.split('-')[-1]}: {title}",
                "body": "\n\n".join(body_parts),
            }
        )
    return records


def graph_titles(ids: list[str]) -> str:
    titles = [item["title"] for item in GRAPH_DEFS if item["id"] in ids]
    return ", ".join(titles)


def formula_titles(ids: list[str]) -> str:
    titles = [item["title"] for item in FORMULA_DEFS if item["id"] in ids]
    return ", ".join(titles)


def split_csv_titles(csv_text: str) -> list[str]:
    if not csv_text:
        return []
    return [part.strip() for part in csv_text.split(",") if part.strip()]


def unique_csv(values: Iterable[str]) -> str:
    seen: set[str] = set()
    ordered: list[str] = []
    for value in values:
        cleaned = sanitize_text(value)
        if not cleaned or cleaned in seen:
            continue
        seen.add(cleaned)
        ordered.append(cleaned)
    return ", ".join(ordered)


def unique_list(values: Iterable[str]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []
    for value in values:
        cleaned = sanitize_text(value)
        if not cleaned or cleaned in seen:
            continue
        seen.add(cleaned)
        ordered.append(cleaned)
    return ordered


def merge_graph_detail_maps(*details: dict | None) -> dict:
    merged: dict = {}
    for detail in details:
        if detail:
            merged.update(detail)
    return merged


def lookup_term_graph_detail(*names: str) -> dict:
    merged: dict = {}
    for name in names:
        if not name:
            continue
        merged = merge_graph_detail_maps(merged, TERM_GRAPH_DETAILS.get(name, {}), TERM_GRAPH_LOCATION_DETAILS.get(name, {}))
    return merged


def load_graph_model_element_map() -> dict[str, dict[str, str]]:
    try:
        text = GRAPH_MODEL_C.read_text(encoding="utf-8")
    except FileNotFoundError:
        return {}

    array_entries: dict[str, dict[str, str]] = {}
    current_array = ""
    array_header = re.compile(r"static const GraphElementEntry (k_[a-z0-9_]+)\[\] = \{")
    entry_line = re.compile(r'\s*\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"')
    map_line = re.compile(r'\s*\{"([^"]+)",\s*(k_[a-z0-9_]+),')

    for line in text.splitlines():
        header_match = array_header.match(line)
        if header_match:
            current_array = header_match.group(1)
            array_entries[current_array] = {}
            continue
        if current_array:
            if line.strip() == "};":
                current_array = ""
                continue
            entry_match = entry_line.match(line)
            if entry_match:
                array_entries[current_array][entry_match.group(1)] = entry_match.group(3)

    graph_elements: dict[str, dict[str, str]] = {}
    for line in text.splitlines():
        map_match = map_line.match(line)
        if not map_match:
            continue
        graph_id = map_match.group(1)
        array_name = map_match.group(2)
        graph_elements[graph_id] = dict(array_entries.get(array_name, {}))
    return graph_elements


GRAPH_MODEL_ELEMENTS = load_graph_model_element_map()


def sanitize_graph_location_ids(location_detail: dict) -> dict:
    graph_id = sanitize_text(location_detail.get("graph_id", ""))
    valid_ids = set(GRAPH_MODEL_ELEMENTS.get(graph_id, {}))
    if not graph_id or not valid_ids:
        return location_detail

    related_curve_ids = [item for item in location_detail.get("related_curve_ids", []) if item in valid_ids]
    related_point_ids = [item for item in location_detail.get("related_point_ids", []) if item in valid_ids]
    related_region_ids = [item for item in location_detail.get("related_region_ids", []) if item in valid_ids]
    graph_element_id = sanitize_text(location_detail.get("graph_element_id", ""))

    if graph_element_id and graph_element_id not in valid_ids:
        graph_element_id = primary_or_empty([*related_point_ids, *related_curve_ids, *related_region_ids])

    if not graph_element_id and not (related_curve_ids or related_point_ids or related_region_ids):
        location_detail["highlight_supported"] = ""
        location_detail["highlight_mode"] = ""

    location_detail["graph_element_id"] = graph_element_id
    location_detail["related_curve_ids"] = related_curve_ids
    location_detail["related_curve_ids_csv"] = unique_csv(related_curve_ids)
    location_detail["related_point_ids"] = related_point_ids
    location_detail["related_point_ids_csv"] = unique_csv(related_point_ids)
    location_detail["related_region_ids"] = related_region_ids
    location_detail["related_region_ids_csv"] = unique_csv(related_region_ids)
    return location_detail


def normalize_graph_location_detail(detail: dict | None, fallback_graph_id: str = "") -> dict:
    detail = detail or {}
    graph_ids = unique_list(detail.get("graph_ids", [detail.get("graph_id", ""), detail.get("graph", ""), fallback_graph_id]))
    graph_id = sanitize_text(detail.get("graph_id", "") or fallback_graph_id or detail.get("graph", "") or primary_or_empty(graph_ids))
    graph_element_type = sanitize_text(detail.get("graph_element_type", "") or detail.get("kind", ""))
    graph_location_text = sanitize_text(detail.get("graph_location_text", "") or detail.get("where", ""))
    highlight_supported = "1" if detail.get("highlight_supported", False) else ""
    highlight_mode = sanitize_text(detail.get("highlight_mode", ""))
    related_curve_ids = unique_list(detail.get("related_curve_ids", []))
    related_point_ids = unique_list(detail.get("related_point_ids", []))
    related_region_ids = unique_list(detail.get("related_region_ids", []))
    return sanitize_graph_location_ids(
        {
        "graph_ids": graph_ids,
        "graph_id": graph_id,
        "graph_element_type": graph_element_type,
        "graph_element_id": sanitize_text(detail.get("graph_element_id", "")),
        "graph_location_text": graph_location_text,
        "highlight_supported": highlight_supported,
        "highlight_mode": highlight_mode,
        "related_curve_ids": related_curve_ids,
        "related_curve_ids_csv": unique_csv(related_curve_ids),
        "related_point_ids": related_point_ids,
        "related_point_ids_csv": unique_csv(related_point_ids),
        "related_region_ids": related_region_ids,
        "related_region_ids_csv": unique_csv(related_region_ids),
        }
    )


def slugify_token(value: str) -> str:
    cleaned = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")
    return re.sub(r"-{2,}", "-", cleaned)


def vocab_id_from_term(term: str) -> str:
    return slugify_token(normalize_term(term))


def primary_or_empty(values: Iterable[str]) -> str:
    for value in values:
        cleaned = sanitize_text(value)
        if cleaned:
            return cleaned
    return ""


def categorize_term(term: str, topic_slug: str, graph_detail: dict | None) -> str:
    if term in TERM_CATEGORY_OVERRIDES:
        return TERM_CATEGORY_OVERRIDES[term]
    lower = term.lower()
    if graph_detail:
        kind = graph_detail.get("kind", "") or graph_detail.get("graph_element_type", "")
        if "region" in kind:
            return "graph area"
        if any(word in kind for word in ["point", "intersection"]):
            return "graph point"
        if any(word in kind for word in ["curve", "line"]):
            return "graph label"
    if "elasticity" in lower:
        return "elasticity term"
    if any(word in lower for word in ["cost", "revenue", "profit", "atc", "avc", "afc", "mc", "mr"]):
        return "cost/revenue term"
    if any(word in lower for word in ["monopoly", "oligopoly", "competition", "monopsony"]):
        return "market structure term"
    if any(word in lower for word in ["labor", "wage", "mrp", "mfc"]):
        return "labor market term"
    if any(word in lower for word in ["tax", "subsidy", "ceiling", "floor"]):
        return "government intervention term"
    if any(word in lower for word in ["externality", "public", "common", "free rider"]):
        return "externality term"
    if any(word in lower for word in ["surplus", "efficiency", "gini", "lorenz", "deadweight"]):
        return "efficiency/welfare term"
    return "core concept"


def term_market_structure(topic_slug: str, term: str) -> str:
    if topic_slug == "perfect-competition":
        return "perfect competition"
    if topic_slug == "monopoly":
        return "monopoly"
    if topic_slug == "monopolistic-competition-oligopoly":
        lower = term.lower()
        if "oligopoly" in lower or "nash" in lower:
            return "oligopoly"
        return "monopolistic competition"
    if topic_slug in {"factor-markets", "factor-markets-labor"} and "monopsony" in term.lower():
        return "monopsony"
    return ""


def pretty_topic_name(topic_slug: str) -> str:
    return topic_slug.replace("-", " ")


def default_vocab_example(term: str, topic_slug: str, category: str) -> dict[str, str]:
    topic_name = pretty_topic_name(topic_slug)
    topic_examples = {
        "scarcity-choice": (
            f"Think about choosing between two good options in a limited day or with a limited budget to see {term.lower()} in action.",
            "This idea usually shows up on a PPC or tradeoff explanation where gaining one thing means giving up some of another."
        ),
        "ppc": (
            f"A factory deciding how much of two products to make can use {term.lower()} to explain tradeoffs and productive limits.",
            "On the PPC, this changes whether a point is inside, on, or beyond the frontier or whether the entire frontier shifts."
        ),
        "comparative-advantage": (
            f"Trade questions use {term.lower()} to explain why specialization can help both sides even when one producer is stronger overall.",
            "This usually changes which side specializes and how a trade or PPC table should be interpreted."
        ),
        "equilibrium": (
            f"A market for tickets, coffee, or gas can use {term.lower()} to explain who gains from trade and why equilibrium matters.",
            "On the graph, it changes how you interpret the equilibrium point and the welfare areas around it."
        ),
        "demand-supply-shifts": (
            f"Changes in tastes, income, input costs, or related-good prices can make {term.lower()} show up in a real market story.",
            "The key graph move is deciding whether the whole curve shifts or whether the market only moves along an existing curve."
        ),
        "elasticity": (
            f"Real products such as gasoline, luxury handbags, or generic cereal help show how {term.lower()} affects responsiveness.",
            "Elasticity changes how steep or flat the curve feels and how much quantity reacts when price changes."
        ),
        "taxes-and-subsidies": (
            f"Government policies on cigarettes, gasoline, or education are common ways to see {term.lower()} in context.",
            "These policies create wedges, change quantity, and often create rectangles or triangles on the graph."
        ),
        "price-controls": (
            f"Rent control or minimum wage examples are common AP ways to apply {term.lower()}.",
            "The graph effect is a legal price line away from equilibrium that creates a shortage or surplus."
        ),
        "production-functions": (
            f"A small firm adding workers to fixed equipment can use {term.lower()} to explain changing productivity.",
            "The graph effect is usually a product curve that eventually flattens or a cost curve that bends upward."
        ),
        "cost-curves": (
            f"Firm production decisions use {term.lower()} to explain whether output is getting cheaper or more expensive at the margin.",
            "The graph effect is a shift in how you read the ATC, AVC, AFC, or MC relationship at a chosen quantity."
        ),
        "perfect-competition": (
            f"Commodity markets such as wheat are the standard AP setting for {term.lower()}.",
            "The graph effect is read on the price-taking firm graph where the horizontal price line meets cost curves."
        ),
        "monopoly": (
            f"A patented drug or local utility can illustrate {term.lower()} under a single seller with market power.",
            "The graph effect is read on a monopoly graph where MR lies below demand and output is restricted."
        ),
        "monopolistic-competition-oligopoly": (
            f"Restaurants, phone carriers, or airlines are common real settings for {term.lower()} depending on the structure involved.",
            "The graph effect usually changes how the firm demand curve, long-run entry, or strategic behavior is interpreted."
        ),
        "factor-markets-labor": (
            f"Hiring decisions at a business can show how {term.lower()} affects labor demand and wages.",
            "The graph effect is usually a shift in labor demand / MRP or a change in the competitive labor equilibrium."
        ),
        "factor-markets": (
            f"A dominant employer in a town can make {term.lower()} especially visible in labor-market questions.",
            "The graph effect is usually on a monopsony graph where hiring and wage differ from the competitive outcome."
        ),
        "externalities": (
            f"Pollution, education, and vaccination examples are standard ways to see {term.lower()} in the real world.",
            "The graph effect is a gap between private and social curves that creates underproduction or overproduction."
        ),
        "public-common-goods": (
            f"National defense, streetlights, fisheries, and public parks are common AP examples for {term.lower()}.",
            "The graph effect is often described with externality-style logic because private incentives diverge from social efficiency."
        ),
        "inequality": (
            f"Income-distribution comparisons between countries or over time can help explain {term.lower()}.",
            "The visual effect is a Lorenz curve moving farther from or closer to the line of equality."
        ),
        "market-failure": (
            f"Whenever private choices do not match what is best for society, AP uses {term.lower()} as part of the explanation.",
            "The graph effect is a quantity that differs from the socially efficient level, creating deadweight loss."
        ),
    }
    example, effect = topic_examples.get(
        topic_slug,
        (
            f"A standard AP Micro scenario can use {term.lower()} to explain decision-making, incentives, or market outcomes.",
            "The graph effect is usually to change which curve shifts, which point matters, or whether quantity is above or below the efficient level."
        ),
    )
    if category == "market structure term":
        effect = "This usually changes which firm graph applies, whether demand is horizontal or downward sloping, and whether price equals marginal revenue."
    elif category == "graph area":
        effect = "The graph effect is usually a shaded triangle or rectangle used to measure gains, losses, or deadweight loss."
    elif category == "graph point":
        effect = "The graph effect is usually a named point or intersection that determines price, quantity, or efficiency."
    return {"example": example, "graph_effect": effect, "visual": effect}


def vocab_usage_text(term: str, topic_slug: str, category: str, graph_detail: dict[str, str]) -> str:
    if graph_detail:
        kind = graph_detail.get("kind", "") or graph_detail.get("graph_element_type", "graph element")
        return (
            f"Use {term} to label or explain the {kind} on AP Micro graphs, "
            "then connect that visual to price, quantity, welfare, or efficiency."
        )
    if category == "market structure term":
        return f"Use {term} to compare firm behavior, price-setting power, and efficiency across market structures."
    if category == "elasticity term":
        return f"Use {term} in responsiveness, tax-incidence, and total-revenue questions."
    if category == "cost/revenue term":
        return f"Use {term} to explain firm output decisions, profit, loss, and shutdown or long-run outcomes."
    if category == "externality term":
        return f"Use {term} to explain why private markets can miss the socially efficient outcome."
    return f"Use {term} in {pretty_topic_name(topic_slug)} questions to identify the concept, explain cause and effect, or justify an AP Micro graph answer."


def build_vocab_explanation(entry: dict, topic_slug: str, graph_detail: dict[str, str], example_detail: dict[str, str]) -> str:
    parts = [entry["definition"]]
    if entry.get("extra"):
        parts.append(entry["extra"])
    if graph_detail:
        kind = graph_detail.get("kind", "") or graph_detail.get("graph_element_type", "key element")
        location = graph_detail.get("where", "") or graph_detail.get("graph_location_text", "")
        meaning = graph_detail.get("meaning", "")
        sentence = f"On AP graphs it appears as a {kind}"
        if location:
            sentence += f" located {location.rstrip('.')}"
        sentence += "."
        if meaning:
            sentence += f" That position matters because it {meaning.rstrip('.')}."
        parts.append(sentence)
    else:
        parts.append(
            f"A concrete way to remember it is this: {example_detail.get('example', '').rstrip('.')}. "
            f"In graph terms, {example_detail.get('graph_effect', '').rstrip('.')}."
        )
    return " ".join(part.strip() for part in parts if part and part.strip())


def build_vocabulary_records(raw_vocab: list[dict]) -> list[dict]:
    records = []
    for entry in raw_vocab:
        term = entry["canonical_term"]
        vocab_id = vocab_id_from_term(term)
        primary_unit_id = entry.get("primary_unit_id", entry.get("unit_id", ""))
        primary_topic_id = entry.get("primary_topic_id", entry.get("topic_slug", ""))
        unit_ids = unique_list(entry.get("unit_ids", [primary_unit_id]))
        topic_ids = unique_list(entry.get("topic_ids", [primary_topic_id]))
        graph_ids = unique_list(
            graph_id
            for topic_slug in topic_ids
            for graph_id in TOPIC_GRAPH_MAP.get(topic_slug, [])
        )
        formula_ids = unique_list(
            formula_id
            for topic_slug in topic_ids
            for formula_id in TOPIC_FORMULA_MAP.get(topic_slug, [])
        )
        graph_detail = lookup_term_graph_detail(term, entry.get("term", ""))
        location_detail = normalize_graph_location_detail(graph_detail)
        graph_ids = unique_list([*graph_ids, *location_detail["graph_ids"]])
        category = categorize_term(term, primary_topic_id, graph_detail)
        if category not in VALID_CATEGORIES:
            category = "core concept"
        short_definition = entry["definition"]
        example_detail = TERM_EXAMPLE_DETAILS.get(term, default_vocab_example(term, primary_topic_id, category))
        long_definition = build_vocab_explanation(entry, primary_topic_id, graph_detail, example_detail)
        used_for = vocab_usage_text(term, primary_topic_id, category, graph_detail)
        question_types = "definition, graph interpretation, multiple-choice reasoning, FRQ explanation"
        if formula_ids:
            question_types += ", calculation"
        related_terms = TERM_RELATED.get(term, TERM_RELATED.get(entry.get("term", ""), entry.get("related", "")))
        confusion = TERM_CONFUSIONS.get(term, "Watch for similar-looking graph labels and AP wording traps.")
        exam_tip = graph_detail.get("meaning", "")
        if not exam_tip:
            exam_tip = f"AP often uses {term} in short explanations, graph labels, or interpretation questions."
        records.append(
            {
                "id": vocab_id,
                "term": term,
                "canonical_term": term,
                "normalized_key": entry.get("normalized_key", normalize_term(term)),
                "source_type": entry.get("source_type", "manual"),
                "aliases_csv": unique_csv(entry.get("aliases", [])),
                "quality_score": str(entry.get("quality_score", 100)),
                "primary_unit_id": primary_unit_id,
                "unit_ids": unit_ids,
                "unit_ids_csv": unique_csv(unit_ids),
                "primary_topic_id": primary_topic_id,
                "topic_ids": topic_ids,
                "topic_ids_csv": unique_csv(topic_ids),
                "category": category,
                "short_definition": short_definition,
                "long_definition": long_definition,
                "used_for": used_for,
                "question_types": question_types,
                "related_graphs": graph_titles(graph_ids),
                "related_graph_ids": graph_ids,
                "related_graph_ids_csv": unique_csv(graph_ids),
                "graph_name": next((item["title"] for item in GRAPH_DEFS if item["id"] == location_detail["graph_id"]), graph_titles(graph_ids)),
                "graph_id": location_detail["graph_id"],
                "graph_kind": graph_detail.get("kind", ""),
                "graph_where": graph_detail.get("where", "") or location_detail["graph_location_text"],
                "graph_meaning": graph_detail.get("meaning", ""),
                "graph_effect": graph_detail.get("effect", ""),
                "graph_element_type": location_detail["graph_element_type"],
                "graph_element_id": location_detail["graph_element_id"],
                "graph_location_text": location_detail["graph_location_text"],
                "highlight_supported": location_detail["highlight_supported"],
                "highlight_mode": location_detail["highlight_mode"],
                "related_curve_ids": location_detail["related_curve_ids"],
                "related_curve_ids_csv": location_detail["related_curve_ids_csv"],
                "related_point_ids": location_detail["related_point_ids"],
                "related_point_ids_csv": location_detail["related_point_ids_csv"],
                "related_region_ids": location_detail["related_region_ids"],
                "related_region_ids_csv": location_detail["related_region_ids_csv"],
                "real_example": example_detail.get("example", ""),
                "example_graph_effect": example_detail.get("graph_effect", graph_detail.get("effect", "")),
                "visual_summary": location_detail["graph_location_text"] or graph_detail.get("where", "") or example_detail.get("visual", ""),
                "related_formulas": formula_titles(formula_ids),
                "related_formula_ids": formula_ids,
                "related_formula_ids_csv": unique_csv(formula_ids),
                "related_terms": related_terms,
                "related_vocab_ids": [],
                "related_vocab_ids_csv": "",
                "related_concept_ids": [],
                "related_concept_ids_csv": "",
                "confusion": confusion,
                "exam_tip": exam_tip,
                "market_structure": term_market_structure(primary_topic_id, term),
            }
        )
    return sorted(records, key=lambda item: item["term"].lower())


def build_topic_records(topics: list[dict], chunks: list[ChunkRecord], vocab: list[dict]) -> list[dict]:
    chunk_map: dict[str, list[ChunkRecord]] = defaultdict(list)
    for chunk in chunks:
        chunk_map[chunk.topic_slug].append(chunk)

    vocab_map: dict[str, list[str]] = defaultdict(list)
    for entry in vocab:
        for topic_id in entry.get("topic_ids", [entry.get("primary_topic_id", "")]):
            if topic_id:
                vocab_map[topic_id].append(entry["term"])

    records = []
    for topic in topics:
        slug = topic["topic_slug"]
        key_ideas = select_topic_key_ideas(slug, chunk_map.get(slug, []))
        if slug in TOPIC_OVERVIEWS:
            overview = TOPIC_OVERVIEWS[slug]
        elif summary_is_noisy(topic["summary"]) and key_ideas:
            overview = " ".join(key_ideas[:2])
        else:
            overview = topic["summary"]
        body_parts = [overview]
        if key_ideas:
            body_parts.append("Key ideas\n- " + "\n- ".join(key_ideas[:6]))
        graphs = TOPIC_GRAPH_MAP.get(slug, [])
        if graphs:
            pretty_graphs = [item["title"] for item in GRAPH_DEFS if item["id"] in graphs]
            body_parts.append("Graphs to review\n- " + "\n- ".join(pretty_graphs))
        formulas = TOPIC_FORMULA_MAP.get(slug, [])
        if formulas:
            pretty_formulas = [item["title"] for item in FORMULA_DEFS if item["id"] in formulas]
            body_parts.append("Formula cues\n- " + "\n- ".join(pretty_formulas))
        topic_vocab = vocab_map.get(slug, [])
        if topic_vocab:
            body_parts.append("Key vocabulary\n- " + "\n- ".join(topic_vocab[:10]))
        records.append(
            {
                "id": slug,
                "primary_unit_id": topic["unit_id"],
                "unit_ids": [topic["unit_id"]],
                "unit_ids_csv": topic["unit_id"],
                "title": topic["title"],
                "body": "\n\n".join(part for part in body_parts if part),
                "related_graphs": graph_titles(graphs),
                "related_graph_ids": graphs,
                "related_graph_ids_csv": unique_csv(graphs),
                "related_formulas": formula_titles(formulas),
                "related_formula_ids": formulas,
                "related_formula_ids_csv": unique_csv(formulas),
                "related_vocab_ids": [],
                "related_vocab_ids_csv": "",
                "related_concept_ids": [],
                "related_concept_ids_csv": "",
            }
        )
    return records


def graph_reading_guide(graph_id: str) -> str:
    guides = {
        "supply-demand": "1. Find the intersection. 2. Read equilibrium price and quantity. 3. Decide whether a curve shifted or price changed on an existing curve. 4. Check how price and quantity changed.",
        "ppc": "1. Identify the frontier. 2. Classify a point as on the curve, inside it, or outside it. 3. Use movement along the frontier to explain opportunity cost. 4. Use a shifted frontier to explain growth or productivity change.",
        "trade-table": "1. Compare total output first for absolute advantage. 2. Compare opportunity costs for comparative advantage. 3. Match each producer to the lower-opportunity-cost good. 4. Check whether the terms of trade fall between the two opportunity costs.",
        "surplus-welfare": "1. Find equilibrium. 2. Shade consumer surplus above price and below demand. 3. Shade producer surplus below price and above supply. 4. Total surplus is both areas together.",
        "tax": "1. Find the original equilibrium. 2. Shift supply up by the tax. 3. Read buyer price, seller price, and new quantity. 4. Rectangle is tax revenue. Triangle is deadweight loss.",
        "subsidy": "1. Find the original equilibrium. 2. Shift supply down by the subsidy. 3. Read buyer price, seller price, and new quantity. 4. Rectangle is government spending. Triangle is overproduction DWL.",
        "price-ceiling": "1. Compare the ceiling to equilibrium price. 2. If it is below equilibrium, it binds. 3. Read quantity supplied and quantity demanded at the ceiling. 4. The gap is shortage.",
        "price-floor": "1. Compare the floor to equilibrium price. 2. If it is above equilibrium, it binds. 3. Read quantity supplied and quantity demanded at the floor. 4. The gap is surplus.",
        "perfect-competition": "1. Use market price as the firm's MR = AR = demand line. 2. Find output where MR = MC. 3. Compare price with ATC for profit and AVC for shutdown.",
        "perfect-competition-market": "1. Use market supply and demand to find price. 2. Carry that market price to the individual firm graph. 3. Analyze the firm's profit or loss from that price.",
        "monopoly": "1. Find where MR = MC for quantity. 2. Move up to demand for price. 3. Compare price with ATC for profit. 4. Compare quantity with the efficient output for DWL.",
        "monopolistic-competition": "1. Find MR = MC output. 2. Read price from demand. 3. Compare price with ATC. 4. In long run, demand is tangent to ATC at the chosen output.",
        "kinked-demand": "1. Identify the kink in the demand curve. 2. Notice the broken MR curve beneath it. 3. If MC moves inside the MR gap, price and output may stay sticky.",
        "game-theory-matrix": "1. Compare payoffs row by row and column by column. 2. Find any dominant strategy. 3. Locate the Nash equilibrium. 4. Compare it with the collusive or joint-profit outcome.",
        "labor-market": "1. Find the intersection of labor demand and labor supply. 2. Read equilibrium wage on the vertical axis and labor quantity on the horizontal axis. 3. Identify whether a shift came from labor demand or labor supply.",
        "hiring-rule": "1. Read the downward-sloping MRP curve as the benefit of another worker. 2. Read the wage = MRC line as the hiring cost. 3. Hire where the two meet.",
        "monopsony": "1. Use MRP = MFC for hiring. 2. Move down to labor supply to read wage. 3. Compare with competitive outcome to see lower wage and employment.",
        "negative-externality": "1. Identify MPC, MSC, and MSB. 2. Find Qm where MPC meets MSB. 3. Find Qsoc where MSC meets MSB. 4. The gap between those quantities is overproduction and creates DWL. 5. Explain how a corrective tax or regulation moves Q toward Qsoc.",
        "positive-externality": "1. Identify MPB, MSB, and MSC. 2. Find Qm where MPB meets MSC. 3. Find Qsoc where MSB meets MSC. 4. The gap between those quantities is underproduction and creates DWL. 5. Explain how a subsidy moves Q toward Qsoc.",
        "public-goods-common-resources": "1. Classify the good by rivalry and excludability. 2. If it is non-rival and non-excludable, think public good and free-rider underprovision. 3. If it is rival and non-excludable, think common resource and overuse. 4. Match the policy to the problem: public provision for public goods, regulation or property-right tools for common resources.",
        "lorenz": "1. Start from the equality line. 2. Compare the Lorenz curve's distance from that line. 3. Greater bowing means more inequality and a higher Gini coefficient.",
        "production-function": "1. Read total product as total output from inputs. 2. Read marginal product as the extra output from one more worker. 3. When MP falls, MC rises.",
    }
    return guides.get(graph_id, "Read axes first, label curves carefully, then identify equilibrium, distortions, or shaded regions.")


def graph_common_questions(graph_id: str) -> str:
    questions = {
        "ppc": "Is the point efficient, inefficient, or unattainable? What is the opportunity cost of moving along the frontier? What causes the PPC to shift outward?",
        "trade-table": "Who has absolute advantage? Who has comparative advantage? Who should specialize in each good? Are there gains from trade?",
        "supply-demand": "Identify equilibrium price and quantity. Explain the effect of a demand or supply shift. Distinguish movement along a curve from a shift.",
        "tax": "Who bears more of the tax? What is tax revenue? What is deadweight loss? How does quantity change?",
        "subsidy": "What is government spending? Where is deadweight loss? Why is output above the efficient level?",
        "price-ceiling": "Is the ceiling binding? What shortage is created? What quantity is actually exchanged?",
        "price-floor": "Is the floor binding? What surplus is created? What quantity is actually exchanged?",
        "perfect-competition": "Find profit-max output, profit/loss, shutdown, and long-run equilibrium.",
        "monopoly": "Find price, quantity, profit, allocative inefficiency, and deadweight loss.",
        "monopolistic-competition": "Find short-run price and quantity, explain long-run zero economic profit, and identify excess capacity.",
        "kinked-demand": "Why might price stay rigid? What happens if MC changes within the MR gap? How does interdependence affect pricing?",
        "game-theory-matrix": "What is each firm's dominant strategy? Where is the Nash equilibrium? Why might the equilibrium be worse than collusion?",
        "labor-market": "Find the equilibrium wage and quantity of labor. Explain what shifts labor demand or labor supply and how the new equilibrium changes.",
        "hiring-rule": "Should the firm hire more labor, less labor, or stay at the current level? How does a wage change or productivity change affect hiring?",
        "negative-externality": "Compare market quantity with socially optimal quantity. Label MPC, MSC, external cost, and deadweight loss. Identify the corrective tax or regulation.",
        "positive-externality": "Show underproduction, label MPB and MSB, and explain how a subsidy or public support can restore efficiency.",
        "public-goods-common-resources": "Is the good a public good or a common resource? Why does the free-rider problem cause underprovision? Why does open access create overuse? What policy could improve the outcome?",
        "lorenz": "Compare equality across economies and explain how the Gini coefficient changes.",
    }
    return questions.get(graph_id, "Identify labels, explain shifts, and connect the graph to AP Micro reasoning.")


def graph_common_mistakes(graph_id: str) -> str:
    mistakes = {
        "ppc": "Do not call a point inside the PPC unattainable. Do not forget that movement along the frontier is different from shifting the whole frontier.",
        "trade-table": "Do not use absolute advantage to decide specialization. Comparative advantage depends on lower opportunity cost.",
        "supply-demand": "Do not say price changes shift demand or supply. Price changes move along the existing curve.",
        "tax": "Do not use pre-tax quantity for tax revenue. Do not assume the legal payer bears the full burden.",
        "subsidy": "Do not place the DWL triangle on the left side of equilibrium. Subsidies create overproduction to the right.",
        "price-ceiling": "Do not confuse shortage with surplus. A low binding price ceiling causes shortage.",
        "price-floor": "Do not confuse surplus with shortage. A high binding price floor causes surplus.",
        "perfect-competition": "Do not use ATC instead of AVC for shutdown. Do not forget MR = price for the firm.",
        "monopoly": "Do not stop at MR = MC and call that price. That only gives quantity.",
        "monopolistic-competition": "Do not call long-run zero economic profit efficient. The firm still has excess capacity and P still exceeds MC.",
        "kinked-demand": "Do not treat the kinked-demand model like an ordinary smooth monopoly graph. The broken MR curve is the point of the model.",
        "game-theory-matrix": "Do not confuse the highest joint payoff with the Nash equilibrium. The equilibrium is about incentives, not fairness.",
        "labor-market": "Do not mix up the market graph with the individual firm's hiring graph. This graph sets the market wage and employment.",
        "hiring-rule": "Do not treat the horizontal wage = MRC line as market supply. It is the firm's given hiring cost in a competitive labor market.",
        "monopsony": "Do not read wage from MFC. Read wage from labor supply at the chosen quantity.",
        "negative-externality": "Do not use private-market equilibrium as the socially efficient quantity. Negative externalities mean overproduction, not underproduction.",
        "positive-externality": "Do not say the market overproduces. Positive externalities cause underproduction, and the efficient quantity is to the right of the market quantity.",
        "public-goods-common-resources": "Do not confuse public goods with common resources. Public goods are non-rival; common resources are rival.",
    }
    return mistakes.get(graph_id, "Label axes, curves, and key points before making claims.")


def build_graph_records(topics: list[dict], vocab: list[dict], formulas: list[dict]) -> list[dict]:
    topic_to_unit = {topic["topic_slug"]: topic["unit_id"] for topic in topics}
    graph_terms: dict[str, list[str]] = defaultdict(list)
    graph_formulas: dict[str, list[str]] = defaultdict(list)
    graph_formula_fallback: dict[str, list[str]] = defaultdict(list)
    graph_title_to_id = {item["title"]: item["id"] for item in GRAPH_DEFS}
    formula_title_to_id = {item["title"]: item["id"] for item in FORMULA_DEFS}
    vocab_id_by_term = {entry["term"]: entry["id"] for entry in vocab}

    for entry in vocab:
        term = entry["term"]
        for title in split_csv_titles(entry.get("related_graphs", "")):
            if title in {item["title"] for item in GRAPH_DEFS}:
                graph_terms[title].append(term)
        if entry.get("graph_name"):
            graph_terms[entry["graph_name"]].append(term)

    for formula in formulas:
        for title in split_csv_titles(formula.get("related_graphs", "")):
            graph_formulas[title].append(formula["title"])

    for topic_slug, graph_ids in TOPIC_GRAPH_MAP.items():
        fallback_titles = split_csv_titles(formula_titles(TOPIC_FORMULA_MAP.get(topic_slug, [])))
        for graph_id in graph_ids:
            graph_formula_fallback[graph_id].extend(fallback_titles)

    records = []
    for item in GRAPH_DEFS:
        topic_ids = [topic_slug for topic_slug, graph_ids in TOPIC_GRAPH_MAP.items() if item["id"] in graph_ids and topic_slug in topic_to_unit]
        unit_ids = unique_list(topic_to_unit.get(topic_slug, "") for topic_slug in topic_ids)
        primary_topic_id = primary_or_empty(topic_ids)
        primary_unit_id = primary_or_empty(unit_ids)
        overview = "\n\n".join(
            [
                f"Used for\n{item['explanation']}",
                f"Axes\n{item['axes']}",
                f"Curves\n{item['curves']}",
                f"Equilibrium / key outcome\n{item['changes']}",
            ]
        )
        labels = "\n\n".join(
            [
                f"Curve labels\n{item['curves']}",
                f"Point and area cues\n{item['notes']}",
            ]
        )
        shifts = "\n\n".join(
            [
                f"Shifts / causes\n{item['shifts']}",
                f"What changes\n{item['changes']}",
                "What does not shift\nPrice alone does not shift the curve being measured; watch for determinant changes instead.",
                "Movement along curves\nA change in the graph's own axis variable moves you along the existing curve.",
            ]
        )
        guide = "\n\n".join(
            [
                "Reading guide",
                graph_reading_guide(item["id"]),
            ]
        )
        questions = "\n\n".join(
            [
                "Common AP questions",
                graph_common_questions(item["id"]),
            ]
        )
        mistakes = "\n\n".join(
            [
                "Common mistakes",
                graph_common_mistakes(item["id"]),
            ]
        )
        related_term_names = unique_csv(graph_terms[item["title"]])
        related_formula_titles = GRAPH_FORMULA_OVERRIDES.get(
            item["id"],
            unique_csv(graph_formulas[item["title"]] + graph_formula_fallback[item["id"]]),
        )
        related_formula_ids = unique_list(formula_title_to_id.get(title, "") for title in split_csv_titles(related_formula_titles))
        related_vocab_ids = unique_list(vocab_id_by_term.get(term, "") for term in split_csv_titles(related_term_names))
        records.append(
            {
                "id": item["id"],
                "title": item["title"],
                "graph_type": item["graph_type"],
                "primary_unit_id": primary_unit_id,
                "unit_ids": unit_ids,
                "unit_ids_csv": unique_csv(unit_ids),
                "primary_topic_id": primary_topic_id,
                "topic_ids": topic_ids,
                "topic_ids_csv": unique_csv(topic_ids),
                "overview_page": overview,
                "labels_page": labels,
                "shifts_page": shifts,
                "guide_page": guide,
                "questions_page": questions,
                "mistakes_page": mistakes,
                "related_terms": related_term_names,
                "related_vocab_ids": related_vocab_ids,
                "related_vocab_ids_csv": unique_csv(related_vocab_ids),
                "related_formulas": related_formula_titles,
                "related_formula_ids": related_formula_ids,
                "related_formula_ids_csv": unique_csv(related_formula_ids),
                "related_concept_ids": [],
                "related_concept_ids_csv": "",
            }
        )
    return records


def build_formula_records(topics: list[dict], vocab: list[dict]) -> list[dict]:
    records = []
    topic_to_unit = {topic["topic_slug"]: topic["unit_id"] for topic in topics}
    formula_topics: dict[str, list[str]] = defaultdict(list)
    vocab_id_by_term = {entry["term"]: entry["id"] for entry in vocab}
    graph_title_to_id = {item["title"]: item["id"] for item in GRAPH_DEFS}
    formula_terms: dict[str, list[str]] = defaultdict(list)
    for topic_slug, formula_ids in TOPIC_FORMULA_MAP.items():
        for formula_id in formula_ids:
            formula_topics[formula_id].append(topic_slug)
    for entry in vocab:
        for title in split_csv_titles(entry.get("related_formulas", "")):
            formula_terms[title].append(entry["term"])
    for item in FORMULA_DEFS:
        topic_ids = unique_list(topic_slug for topic_slug in formula_topics.get(item["id"], []) if topic_slug in topic_to_unit)
        primary_topic_id = primary_or_empty(topic_ids)
        unit_ids = unique_list(topic_to_unit.get(topic_slug, "") for topic_slug in topic_ids)
        primary_unit_id = primary_or_empty(unit_ids)
        graph_ids = unique_list(
            graph_id
            for topic_slug in topic_ids
            for graph_id in TOPIC_GRAPH_MAP.get(topic_slug, [])
        )
        when_context = primary_topic_id.replace("-", " ") if primary_topic_id else "multiple topic"
        when_to_use = f"Use this in {when_context} questions when AP asks you to calculate or interpret a numerical result."
        interpretation = "After calculating, explain what the number means for responsiveness, revenue, cost, profit, or efficiency."
        trap = item["tip"]
        body = "\n\n".join(
            [
                f"Formula\n{item['formula']}",
                f"Variables\n{item['variables']}",
                f"When to use it\n{when_to_use}",
                f"How to interpret it\n{interpretation}",
                f"Common trap\n{trap}",
            ]
        )
        related_term_names = unique_csv(formula_terms[item["title"]])
        related_vocab_ids = unique_list(vocab_id_by_term.get(term, "") for term in split_csv_titles(related_term_names))
        records.append(
            {
                "id": item["id"],
                "title": item["title"],
                "helper": item["helper"],
                "primary_unit_id": primary_unit_id,
                "unit_ids": unit_ids,
                "unit_ids_csv": unique_csv(unit_ids),
                "primary_topic_id": primary_topic_id,
                "topic_ids": topic_ids,
                "topic_ids_csv": unique_csv(topic_ids),
                "formula": item["formula"],
                "variables": item["variables"],
                "when_to_use": when_to_use,
                "interpretation": interpretation,
                "trap": trap,
                "related_terms": related_term_names,
                "related_vocab_ids": related_vocab_ids,
                "related_vocab_ids_csv": unique_csv(related_vocab_ids),
                "related_graphs": graph_titles(graph_ids),
                "related_graph_ids": graph_ids,
                "related_graph_ids_csv": unique_csv(graph_ids),
                "related_concept_ids": [],
                "related_concept_ids_csv": "",
                "body": body,
            }
        )
    return records


def build_structure_records() -> list[dict]:
    return MARKET_STRUCTURES


def infer_concept_primary_topic_id(concept: dict, topics: list[dict]) -> str:
    explicit = sanitize_text(concept.get("primary_topic_id", ""))
    if explicit:
        return explicit
    explicit_ids = concept.get("topic_ids", [])
    if explicit_ids:
        return sanitize_text(explicit_ids[0])
    if concept["id"].startswith("concept-"):
        candidate = concept["id"][len("concept-"):]
        if any(topic["topic_slug"] == candidate for topic in topics):
            return candidate
    return ""


def build_concept_records(topics: list[dict], vocab: list[dict], graphs: list[dict], formulas: list[dict]) -> list[dict]:
    topic_to_unit = {topic["topic_slug"]: topic["unit_id"] for topic in topics}
    vocab_id_by_term = {entry["term"]: entry["id"] for entry in vocab}
    formula_id_by_title = {entry["title"]: entry["id"] for entry in formulas}

    records = []
    for item in CONCEPT_DEFS:
        primary_unit_id = item["unit_id"]
        primary_topic_id = infer_concept_primary_topic_id(item, topics)
        topic_ids = unique_list(item.get("topic_ids", [primary_topic_id] if primary_topic_id else []))
        unit_ids = unique_list(item.get("unit_ids", [primary_unit_id] + [topic_to_unit.get(topic_id, "") for topic_id in topic_ids]))
        related_formula_ids = unique_list(formula_id_by_title.get(title, "") for title in split_csv_titles(item.get("related_formulas", "")))
        related_vocab_ids = unique_list(vocab_id_by_term.get(term, "") for term in split_csv_titles(item.get("related_terms", "")))
        graph_detail = merge_graph_detail_maps(
            lookup_term_graph_detail(item.get("title", "")),
            lookup_term_graph_detail(item.get("graph_focus_term", "")),
            CONCEPT_GRAPH_LOCATION_DETAILS.get(item["id"], {}),
        )
        location_detail = normalize_graph_location_detail(graph_detail, item.get("graph_id", ""))
        related_graph_ids = unique_list([item.get("graph_id", ""), *location_detail["graph_ids"]])
        records.append(
            {
                "id": item["id"],
                "title": item["title"],
                "primary_unit_id": primary_unit_id,
                "unit_ids": unit_ids,
                "unit_ids_csv": unique_csv(unit_ids),
                "primary_topic_id": primary_topic_id,
                "topic_ids": topic_ids,
                "topic_ids_csv": unique_csv(topic_ids),
                "short_definition": item["short_definition"],
                "full_explanation": item["full_explanation"],
                "why_it_matters": item["why_it_matters"],
                "exam_use": item["exam_use"],
                "real_world_example": item["real_world_example"],
                "graph_connection": item["graph_connection"],
                "graph_id": item["graph_id"] or location_detail["graph_id"],
                "graph_focus_term": item["graph_focus_term"],
                "graph_element_type": location_detail["graph_element_type"],
                "graph_element_id": location_detail["graph_element_id"],
                "graph_location_text": location_detail["graph_location_text"],
                "highlight_supported": location_detail["highlight_supported"],
                "highlight_mode": location_detail["highlight_mode"],
                "related_curve_ids": location_detail["related_curve_ids"],
                "related_curve_ids_csv": location_detail["related_curve_ids_csv"],
                "related_point_ids": location_detail["related_point_ids"],
                "related_point_ids_csv": location_detail["related_point_ids_csv"],
                "related_region_ids": location_detail["related_region_ids"],
                "related_region_ids_csv": location_detail["related_region_ids_csv"],
                "related_terms": item["related_terms"],
                "related_vocab_ids": related_vocab_ids,
                "related_vocab_ids_csv": unique_csv(related_vocab_ids),
                "related_formulas": item["related_formulas"],
                "related_formula_ids": related_formula_ids,
                "related_formula_ids_csv": unique_csv(related_formula_ids),
                "related_graph_ids": related_graph_ids,
                "related_graph_ids_csv": unique_csv(related_graph_ids),
                "quick_recall": item["quick_recall"],
            }
        )
    return records


def build_scan_pages(scan_report: dict) -> list[dict]:
    body = "\n\n".join(
        [
            f"Database\n{scan_report['database']}",
            "Slide decks\n- " + "\n- ".join(Path(path).name for path in scan_report["powerpoints"]),
            "Documents\n- " + "\n- ".join(Path(path).name for path in scan_report["documents"]),
            "Desktop graph modules\n- " + "\n- ".join(Path(path).name for path in scan_report["graph_modules"]),
        ]
    )
    return [
        {
            "id": "source-audit",
            "title": "Desktop Source Audit",
            "body": body,
        }
    ]


def enrich_cross_links(payload: dict) -> None:
    vocab_by_term = {entry["term"]: entry for entry in payload["vocabulary"]}
    vocab_by_id = {entry["id"]: entry for entry in payload["vocabulary"]}
    graph_by_id = {entry["id"]: entry for entry in payload["graphs"]}
    formula_by_id = {entry["id"]: entry for entry in payload["formulas"]}
    concept_by_id = {entry["id"]: entry for entry in payload["concepts"]}

    for entry in payload["vocabulary"]:
        related_vocab_ids = unique_list(vocab_by_term.get(term, {}).get("id", "") for term in split_csv_titles(entry.get("related_terms", "")))
        related_concept_ids = unique_list(
            concept["id"]
            for concept in payload["concepts"]
            if entry["term"] in split_csv_titles(concept.get("related_terms", ""))
        )
        entry["related_vocab_ids"] = related_vocab_ids
        entry["related_vocab_ids_csv"] = unique_csv(related_vocab_ids)
        entry["related_concept_ids"] = related_concept_ids
        entry["related_concept_ids_csv"] = unique_csv(related_concept_ids)

    for entry in payload["topics"]:
        related_vocab_ids = unique_list(
            vocab["id"]
            for vocab in payload["vocabulary"]
            if entry["id"] in vocab.get("topic_ids", [])
        )
        related_concept_ids = unique_list(
            concept["id"]
            for concept in payload["concepts"]
            if entry["id"] in concept.get("topic_ids", [])
        )
        entry["related_vocab_ids"] = related_vocab_ids
        entry["related_vocab_ids_csv"] = unique_csv(related_vocab_ids)
        entry["related_concept_ids"] = related_concept_ids
        entry["related_concept_ids_csv"] = unique_csv(related_concept_ids)

    for entry in payload["graphs"]:
        related_concept_ids = unique_list(
            concept["id"]
            for concept in payload["concepts"]
            if entry["id"] == concept.get("graph_id", "") or entry["id"] in concept.get("related_graph_ids", [])
        )
        entry["related_concept_ids"] = related_concept_ids
        entry["related_concept_ids_csv"] = unique_csv(related_concept_ids)

    for entry in payload["formulas"]:
        related_concept_ids = unique_list(
            concept["id"]
            for concept in payload["concepts"]
            if entry["id"] in concept.get("related_formula_ids", [])
        )
        entry["related_concept_ids"] = related_concept_ids
        entry["related_concept_ids_csv"] = unique_csv(related_concept_ids)

    for entry in payload["concepts"]:
        if not entry.get("related_graph_ids"):
            entry["related_graph_ids"] = unique_list([entry.get("graph_id", "")])
            entry["related_graph_ids_csv"] = unique_csv(entry["related_graph_ids"])
        if not entry.get("related_formula_ids"):
            entry["related_formula_ids"] = []
            entry["related_formula_ids_csv"] = ""
        if not entry.get("related_vocab_ids"):
            related_vocab_ids = unique_list(vocab_by_term.get(term, {}).get("id", "") for term in split_csv_titles(entry.get("related_terms", "")))
            entry["related_vocab_ids"] = related_vocab_ids
            entry["related_vocab_ids_csv"] = unique_csv(related_vocab_ids)


def sanitize_payload_relationships(payload: dict) -> dict[str, int]:
    graph_by_id = {entry["id"]: entry for entry in payload["graphs"]}
    graph_title_by_id = {entry["id"]: entry["title"] for entry in payload["graphs"]}
    graph_id_by_title = {entry["title"]: entry["id"] for entry in payload["graphs"]}
    formula_by_id = {entry["id"]: entry for entry in payload["formulas"]}
    formula_title_by_id = {entry["id"]: entry["title"] for entry in payload["formulas"]}
    formula_id_by_title = {entry["title"]: entry["id"] for entry in payload["formulas"]}
    vocab_by_id = {entry["id"]: entry for entry in payload["vocabulary"]}
    vocab_term_by_id = {entry["id"]: entry["term"] for entry in payload["vocabulary"]}
    concept_by_id = {entry["id"]: entry for entry in payload["concepts"]}
    counts = {
        "graph_ids_filtered": 0,
        "formula_ids_filtered": 0,
        "vocab_ids_filtered": 0,
        "concept_ids_filtered": 0,
        "graph_titles_filtered": 0,
        "formula_titles_filtered": 0,
        "graph_focus_sanitized": 0,
    }

    def filter_ids(entry: dict, field: str, valid_ids: dict, count_key: str) -> None:
        original = unique_list(entry.get(field, []))
        filtered = [item for item in original if item in valid_ids]
        if len(filtered) != len(original):
            counts[count_key] += len(original) - len(filtered)
        entry[field] = filtered
        entry[f"{field}_csv"] = unique_csv(filtered)

    def filter_titles(entry: dict, field: str, title_to_id: dict, valid_ids: dict, count_key: str) -> None:
        original = split_csv_titles(entry.get(field, ""))
        filtered = [title for title in original if title_to_id.get(title, "") in valid_ids]
        if len(filtered) != len(original):
            counts[count_key] += len(original) - len(filtered)
        entry[field] = unique_csv(filtered)

    for entry in payload["topics"]:
        filter_ids(entry, "related_graph_ids", graph_by_id, "graph_ids_filtered")
        filter_ids(entry, "related_formula_ids", formula_by_id, "formula_ids_filtered")
        filter_ids(entry, "related_vocab_ids", vocab_by_id, "vocab_ids_filtered")
        filter_ids(entry, "related_concept_ids", concept_by_id, "concept_ids_filtered")
        if not entry["related_graph_ids"]:
            filter_titles(entry, "related_graphs", graph_id_by_title, graph_by_id, "graph_titles_filtered")
        else:
            entry["related_graphs"] = unique_csv(graph_title_by_id[item] for item in entry["related_graph_ids"])
        if not entry["related_formula_ids"]:
            filter_titles(entry, "related_formulas", formula_id_by_title, formula_by_id, "formula_titles_filtered")
        else:
            entry["related_formulas"] = unique_csv(formula_title_by_id[item] for item in entry["related_formula_ids"])

    for entry in payload["vocabulary"]:
        previous_graph_id = entry.get("graph_id", "")
        previous_graph_name = entry.get("graph_name", "")
        previous_graph_element_id = entry.get("graph_element_id", "")
        previous_highlight_supported = entry.get("highlight_supported", "")
        previous_highlight_mode = entry.get("highlight_mode", "")
        filter_ids(entry, "related_graph_ids", graph_by_id, "graph_ids_filtered")
        filter_ids(entry, "related_formula_ids", formula_by_id, "formula_ids_filtered")
        filter_ids(entry, "related_vocab_ids", vocab_by_id, "vocab_ids_filtered")
        filter_ids(entry, "related_concept_ids", concept_by_id, "concept_ids_filtered")
        if entry.get("graph_id", "") and entry["graph_id"] not in graph_by_id:
            entry["graph_id"] = ""
        if entry.get("graph_id", ""):
            entry["graph_name"] = graph_title_by_id[entry["graph_id"]]
            if entry["graph_id"] not in entry["related_graph_ids"]:
                entry["related_graph_ids"] = unique_list([entry["graph_id"], *entry["related_graph_ids"]])
                entry["related_graph_ids_csv"] = unique_csv(entry["related_graph_ids"])
        elif entry.get("graph_name", ""):
            resolved_graph_id = graph_id_by_title.get(entry["graph_name"], "")
            if resolved_graph_id:
                entry["graph_id"] = resolved_graph_id
                entry["graph_name"] = graph_title_by_id[resolved_graph_id]
                entry["related_graph_ids"] = unique_list([resolved_graph_id, *entry["related_graph_ids"]])
                entry["related_graph_ids_csv"] = unique_csv(entry["related_graph_ids"])
            else:
                entry["graph_name"] = ""
        entry["related_graphs"] = unique_csv(graph_title_by_id[item] for item in entry["related_graph_ids"])
        entry.update(sanitize_graph_location_ids(entry))
        if not entry.get("graph_id", "") or not entry.get("graph_element_id", ""):
            entry["highlight_supported"] = ""
            entry["highlight_mode"] = ""
        if (
            entry.get("graph_id", "") != previous_graph_id or
            entry.get("graph_name", "") != previous_graph_name or
            entry.get("graph_element_id", "") != previous_graph_element_id or
            entry.get("highlight_supported", "") != previous_highlight_supported or
            entry.get("highlight_mode", "") != previous_highlight_mode
        ):
            counts["graph_focus_sanitized"] += 1

    for entry in payload["concepts"]:
        previous_graph_id = entry.get("graph_id", "")
        previous_graph_element_id = entry.get("graph_element_id", "")
        previous_highlight_supported = entry.get("highlight_supported", "")
        previous_highlight_mode = entry.get("highlight_mode", "")
        filter_ids(entry, "related_graph_ids", graph_by_id, "graph_ids_filtered")
        filter_ids(entry, "related_formula_ids", formula_by_id, "formula_ids_filtered")
        filter_ids(entry, "related_vocab_ids", vocab_by_id, "vocab_ids_filtered")
        if entry.get("graph_id", "") and entry["graph_id"] not in graph_by_id:
            entry["graph_id"] = ""
        if entry.get("graph_id", ""):
            if entry["graph_id"] not in entry["related_graph_ids"]:
                entry["related_graph_ids"] = unique_list([entry["graph_id"], *entry["related_graph_ids"]])
        elif entry["related_graph_ids"]:
            entry["graph_id"] = entry["related_graph_ids"][0]
        entry["related_graph_ids_csv"] = unique_csv(entry["related_graph_ids"])
        entry.update(sanitize_graph_location_ids(entry))
        if not entry.get("graph_id", "") or not entry.get("graph_element_id", ""):
            entry["highlight_supported"] = ""
            entry["highlight_mode"] = ""
        if (
            entry.get("graph_id", "") != previous_graph_id or
            entry.get("graph_element_id", "") != previous_graph_element_id or
            entry.get("highlight_supported", "") != previous_highlight_supported or
            entry.get("highlight_mode", "") != previous_highlight_mode
        ):
            counts["graph_focus_sanitized"] += 1

    for entry in payload["graphs"]:
        filter_ids(entry, "related_vocab_ids", vocab_by_id, "vocab_ids_filtered")
        filter_ids(entry, "related_formula_ids", formula_by_id, "formula_ids_filtered")
        filter_ids(entry, "related_concept_ids", concept_by_id, "concept_ids_filtered")
        entry["related_terms"] = unique_csv(vocab_term_by_id[item] for item in entry["related_vocab_ids"])
        entry["related_formulas"] = unique_csv(formula_title_by_id[item] for item in entry["related_formula_ids"])

    for entry in payload["formulas"]:
        filter_ids(entry, "related_graph_ids", graph_by_id, "graph_ids_filtered")
        filter_ids(entry, "related_vocab_ids", vocab_by_id, "vocab_ids_filtered")
        filter_ids(entry, "related_concept_ids", concept_by_id, "concept_ids_filtered")
        entry["related_graphs"] = unique_csv(graph_title_by_id[item] for item in entry["related_graph_ids"])
        entry["related_terms"] = unique_csv(vocab_term_by_id[item] for item in entry["related_vocab_ids"])

    return counts


def c_escape(text: str) -> str:
    return (
        text.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "")
    )


def emit_c_array(name: str, struct_name: str, fields: list[str], records: list[dict]) -> str:
    lines = [f"const {struct_name} {name}[{len(records)}] = {{"]
    for record in records:
        values = []
        for field in fields:
            value = record.get(field, "")
            values.append(f'"{c_escape(str(value))}"')
        lines.append("    {" + ", ".join(values) + "},")
    lines.append("};")
    return "\n".join(lines)


def emit_header(counts: dict[str, int]) -> str:
    return f"""#ifndef APMICRO_CONTENT_H
#define APMICRO_CONTENT_H

typedef struct {{
    const char *id;
    const char *title;
    const char *body;
}} UnitEntry;

typedef struct {{
    const char *id;
    const char *primary_unit_id;
    const char *unit_ids_csv;
    const char *title;
    const char *body;
    const char *related_graphs;
    const char *related_graph_ids_csv;
    const char *related_formulas;
    const char *related_formula_ids_csv;
    const char *related_vocab_ids_csv;
    const char *related_concept_ids_csv;
}} TopicEntry;

typedef struct {{
    const char *id;
    const char *term;
    const char *canonical_term;
    const char *normalized_key;
    const char *source_type;
    const char *aliases_csv;
    const char *quality_score;
    const char *primary_unit_id;
    const char *unit_ids_csv;
    const char *primary_topic_id;
    const char *topic_ids_csv;
    const char *category;
    const char *short_definition;
    const char *long_definition;
    const char *used_for;
    const char *question_types;
    const char *related_graphs;
    const char *related_graph_ids_csv;
    const char *graph_name;
    const char *graph_id;
    const char *graph_element_type;
    const char *graph_element_id;
    const char *graph_location_text;
    const char *highlight_supported;
    const char *highlight_mode;
    const char *related_curve_ids_csv;
    const char *related_point_ids_csv;
    const char *related_region_ids_csv;
    const char *graph_kind;
    const char *graph_where;
    const char *graph_meaning;
    const char *graph_effect;
    const char *real_example;
    const char *example_graph_effect;
    const char *visual_summary;
    const char *related_formulas;
    const char *related_formula_ids_csv;
    const char *related_terms;
    const char *related_vocab_ids_csv;
    const char *related_concept_ids_csv;
    const char *confusion;
    const char *exam_tip;
    const char *market_structure;
}} VocabularyEntry;

typedef struct {{
    const char *id;
    const char *title;
    const char *primary_unit_id;
    const char *unit_ids_csv;
    const char *primary_topic_id;
    const char *topic_ids_csv;
    const char *short_definition;
    const char *full_explanation;
    const char *why_it_matters;
    const char *exam_use;
    const char *real_world_example;
    const char *graph_connection;
    const char *graph_id;
    const char *related_graph_ids_csv;
    const char *graph_focus_term;
    const char *graph_element_type;
    const char *graph_element_id;
    const char *graph_location_text;
    const char *highlight_supported;
    const char *highlight_mode;
    const char *related_curve_ids_csv;
    const char *related_point_ids_csv;
    const char *related_region_ids_csv;
    const char *related_terms;
    const char *related_vocab_ids_csv;
    const char *related_formulas;
    const char *related_formula_ids_csv;
    const char *quick_recall;
}} ConceptEntry;

typedef struct {{
    const char *id;
    const char *title;
    const char *graph_type;
    const char *primary_unit_id;
    const char *unit_ids_csv;
    const char *primary_topic_id;
    const char *topic_ids_csv;
    const char *overview_page;
    const char *labels_page;
    const char *shifts_page;
    const char *guide_page;
    const char *questions_page;
    const char *mistakes_page;
    const char *related_terms;
    const char *related_vocab_ids_csv;
    const char *related_formulas;
    const char *related_formula_ids_csv;
    const char *related_concept_ids_csv;
}} GraphEntry;

typedef struct {{
    const char *id;
    const char *title;
    const char *helper;
    const char *primary_unit_id;
    const char *unit_ids_csv;
    const char *primary_topic_id;
    const char *topic_ids_csv;
    const char *formula;
    const char *variables;
    const char *when_to_use;
    const char *interpretation;
    const char *trap;
    const char *related_terms;
    const char *related_vocab_ids_csv;
    const char *related_graphs;
    const char *related_graph_ids_csv;
    const char *related_concept_ids_csv;
    const char *body;
}} FormulaEntry;

typedef struct {{
    const char *id;
    const char *title;
    const char *body;
}} TextSheetEntry;

typedef struct {{
    const char *id;
    const char *title;
    const char *summary;
    const char *body;
    const char *related_graphs;
    const char *related_terms;
}} StructureEntry;

#define UNIT_COUNT {counts['units']}
#define TOPIC_COUNT {counts['topics']}
#define VOCAB_COUNT {counts['vocab']}
#define GRAPH_COUNT {counts['graphs']}
#define FORMULA_COUNT {counts['formulas']}
#define CONCEPT_COUNT {counts['concepts']}
#define STRUCTURE_COUNT {counts['structures']}
#define QUICK_REVIEW_COUNT {counts['quick_review']}
#define EXAM_CRAM_COUNT {counts['exam_cram']}
#define REFERENCE_COUNT {counts['reference']}
#define AUDIT_COUNT {counts['audit']}

extern const UnitEntry g_units[UNIT_COUNT];
extern const TopicEntry g_topics[TOPIC_COUNT];
extern const VocabularyEntry g_vocabulary[VOCAB_COUNT];
extern const GraphEntry g_graphs[GRAPH_COUNT];
extern const FormulaEntry g_formulas[FORMULA_COUNT];
extern const ConceptEntry g_concepts[CONCEPT_COUNT];
extern const StructureEntry g_structures[STRUCTURE_COUNT];
extern const TextSheetEntry g_quick_review[QUICK_REVIEW_COUNT];
extern const TextSheetEntry g_exam_cram[EXAM_CRAM_COUNT];
extern const TextSheetEntry g_reference[REFERENCE_COUNT];
extern const TextSheetEntry g_source_audit[AUDIT_COUNT];

#endif
"""


def emit_source(payload: dict) -> str:
    sections = [
        '#include "apmicro_content.h"\n',
        emit_c_array("g_units", "UnitEntry", ["id", "title", "body"], payload["units"]),
        "",
        emit_c_array(
            "g_topics",
            "TopicEntry",
            [
                "id",
                "primary_unit_id",
                "unit_ids_csv",
                "title",
                "body",
                "related_graphs",
                "related_graph_ids_csv",
                "related_formulas",
                "related_formula_ids_csv",
                "related_vocab_ids_csv",
                "related_concept_ids_csv",
            ],
            payload["topics"],
        ),
        "",
        emit_c_array(
            "g_vocabulary",
            "VocabularyEntry",
            [
                "id",
                "term",
                "canonical_term",
                "normalized_key",
                "source_type",
                "aliases_csv",
                "quality_score",
                "primary_unit_id",
                "unit_ids_csv",
                "primary_topic_id",
                "topic_ids_csv",
                "category",
                "short_definition",
                "long_definition",
                "used_for",
                "question_types",
                "related_graphs",
                "related_graph_ids_csv",
                "graph_name",
                "graph_id",
                "graph_element_type",
                "graph_element_id",
                "graph_location_text",
                "highlight_supported",
                "highlight_mode",
                "related_curve_ids_csv",
                "related_point_ids_csv",
                "related_region_ids_csv",
                "graph_kind",
                "graph_where",
                "graph_meaning",
                "graph_effect",
                "real_example",
                "example_graph_effect",
                "visual_summary",
                "related_formulas",
                "related_formula_ids_csv",
                "related_terms",
                "related_vocab_ids_csv",
                "related_concept_ids_csv",
                "confusion",
                "exam_tip",
                "market_structure",
            ],
            payload["vocabulary"],
        ),
        "",
        emit_c_array(
            "g_graphs",
            "GraphEntry",
            [
                "id",
                "title",
                "graph_type",
                "primary_unit_id",
                "unit_ids_csv",
                "primary_topic_id",
                "topic_ids_csv",
                "overview_page",
                "labels_page",
                "shifts_page",
                "guide_page",
                "questions_page",
                "mistakes_page",
                "related_terms",
                "related_vocab_ids_csv",
                "related_formulas",
                "related_formula_ids_csv",
                "related_concept_ids_csv",
            ],
            payload["graphs"],
        ),
        "",
        emit_c_array(
            "g_formulas",
            "FormulaEntry",
            [
                "id",
                "title",
                "helper",
                "primary_unit_id",
                "unit_ids_csv",
                "primary_topic_id",
                "topic_ids_csv",
                "formula",
                "variables",
                "when_to_use",
                "interpretation",
                "trap",
                "related_terms",
                "related_vocab_ids_csv",
                "related_graphs",
                "related_graph_ids_csv",
                "related_concept_ids_csv",
                "body",
            ],
            payload["formulas"],
        ),
        "",
        emit_c_array(
            "g_concepts",
            "ConceptEntry",
            [
                "id",
                "title",
                "primary_unit_id",
                "unit_ids_csv",
                "primary_topic_id",
                "topic_ids_csv",
                "short_definition",
                "full_explanation",
                "why_it_matters",
                "exam_use",
                "real_world_example",
                "graph_connection",
                "graph_id",
                "related_graph_ids_csv",
                "graph_focus_term",
                "graph_element_type",
                "graph_element_id",
                "graph_location_text",
                "highlight_supported",
                "highlight_mode",
                "related_curve_ids_csv",
                "related_point_ids_csv",
                "related_region_ids_csv",
                "related_terms",
                "related_vocab_ids_csv",
                "related_formulas",
                "related_formula_ids_csv",
                "quick_recall",
            ],
            payload["concepts"],
        ),
        "",
        emit_c_array("g_structures", "StructureEntry", ["id", "title", "summary", "body", "related_graphs", "related_terms"], payload["structures"]),
        "",
        emit_c_array("g_quick_review", "TextSheetEntry", ["id", "title", "body"], payload["quick_review"]),
        "",
        emit_c_array("g_exam_cram", "TextSheetEntry", ["id", "title", "body"], payload["exam_cram"]),
        "",
        emit_c_array("g_reference", "TextSheetEntry", ["id", "title", "body"], payload["reference"]),
        "",
        emit_c_array("g_source_audit", "TextSheetEntry", ["id", "title", "body"], payload["audit"]),
        "",
    ]
    return "\n".join(sections)


def main() -> None:
    source_root = resolve_source_root()
    scan_report, source_paths = scan_repo_sources(source_root)
    db_path = source_paths["database"]
    topics, chunks = load_db_topics(db_path)
    topics = merge_manual_topics(topics)
    vocab_pipeline = build_vocab_pipeline(chunks)
    vocab = build_vocabulary_records(vocab_pipeline["merged_vocab_output"])

    units = build_unit_records(topics, vocab)
    topic_records = build_topic_records(topics, chunks, vocab)
    formula_records = build_formula_records(topics, vocab)
    graph_records = build_graph_records(topics, vocab, formula_records)
    concept_records = build_concept_records(topics, vocab, graph_records, formula_records)
    structure_records = build_structure_records()
    audit_records = build_scan_pages(scan_report)

    office_preview: dict[str, list[str]] = {}
    for source_path in [*source_paths["powerpoints"][:4], *source_paths["documents"][:2]]:
        extracted = extract_office_text(source_path)
        office_preview[source_path.name] = [sanitize_text(item)[:200] for item in extracted[:2]]

    payload = {
        "scan_report": scan_report,
        "office_preview": office_preview,
        "vocab_pipeline": vocab_pipeline["report"],
        "units": units,
        "topics": topic_records,
        "vocabulary": vocab,
        "graphs": graph_records,
        "formulas": formula_records,
        "concepts": concept_records,
        "structures": structure_records,
        "quick_review": QUICK_REVIEW,
        "exam_cram": EXAM_CRAM,
        "reference": REFERENCE_SHEETS,
        "audit": audit_records,
    }

    enrich_cross_links(payload)
    sanitization_report = sanitize_payload_relationships(payload)
    payload["sanitization_report"] = sanitization_report

    OUTPUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_C.parent.mkdir(parents=True, exist_ok=True)

    OUTPUT_JSON.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    counts = {
        "units": len(units),
        "topics": len(topic_records),
        "vocab": len(vocab),
        "graphs": len(graph_records),
        "formulas": len(formula_records),
        "concepts": len(concept_records),
        "structures": len(structure_records),
        "quick_review": len(QUICK_REVIEW),
        "exam_cram": len(EXAM_CRAM),
        "reference": len(REFERENCE_SHEETS),
        "audit": len(audit_records),
    }
    OUTPUT_H.write_text(emit_header(counts), encoding="utf-8")
    OUTPUT_C.write_text(emit_source(payload), encoding="utf-8")

    print(f"Wrote {OUTPUT_JSON}")
    print(f"Wrote {OUTPUT_H}")
    print(f"Wrote {OUTPUT_C}")
    print(f"Sanitized links: {sanitization_report}")
    print(
        "Counts: "
        f"units={counts['units']} topics={counts['topics']} vocab={counts['vocab']} "
        f"graphs={counts['graphs']} formulas={counts['formulas']} concepts={counts['concepts']} structures={counts['structures']}"
    )


if __name__ == "__main__":
    main()
