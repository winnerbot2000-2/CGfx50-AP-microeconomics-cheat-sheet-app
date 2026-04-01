#ifndef APMICRO_CONTENT_H
#define APMICRO_CONTENT_H

typedef struct {
    const char *id;
    const char *title;
    const char *body;
} UnitEntry;

typedef struct {
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
} TopicEntry;

typedef struct {
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
} VocabularyEntry;

typedef struct {
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
} ConceptEntry;

typedef struct {
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
} GraphEntry;

typedef struct {
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
} FormulaEntry;

typedef struct {
    const char *id;
    const char *title;
    const char *body;
} TextSheetEntry;

typedef struct {
    const char *id;
    const char *title;
    const char *summary;
    const char *body;
    const char *related_graphs;
    const char *related_terms;
} StructureEntry;

#define UNIT_COUNT 6
#define TOPIC_COUNT 43
#define VOCAB_COUNT 150
#define GRAPH_COUNT 23
#define FORMULA_COUNT 18
#define CONCEPT_COUNT 31
#define STRUCTURE_COUNT 4
#define QUICK_REVIEW_COUNT 7
#define EXAM_CRAM_COUNT 5
#define REFERENCE_COUNT 10
#define AUDIT_COUNT 1

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
