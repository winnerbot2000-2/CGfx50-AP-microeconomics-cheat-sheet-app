#include "graph_model.h"

#include <string.h>

static const GraphElementEntry k_supply_demand_elements[] = {
    {"demand", "Demand Curve", "curve", "Downward-sloping curve", "Shows buyers' willingness to buy at each price.", "Identify movement along demand and demand shifts.", "Market Equilibrium, Consumer Surplus, Shortage, Surplus", "Price Elasticity of Demand, Total Revenue", false, true, 212, 152},
    {"supply", "Supply Curve", "curve", "Upward-sloping curve", "Shows sellers' willingness to produce at each price.", "Identify movement along supply and supply shifts.", "Market Equilibrium, Producer Surplus", "Price Elasticity of Supply", false, true, 214, 68},
    {"equilibrium", "Equilibrium", "point", "Intersection of demand and supply", "Where quantity demanded equals quantity supplied.", "Find equilibrium price and equilibrium quantity.", "Market Equilibrium", "Total Revenue, Deadweight Loss", false, true, 156, 114},
    {"consumer-surplus", "Consumer Surplus", "region", "Above price and below demand", "Measures buyer surplus from trade.", "Used in welfare and total-surplus questions.", "Consumer Surplus", "Deadweight Loss", true, false, 104, 82},
    {"producer-surplus", "Producer Surplus", "region", "Below price and above supply", "Measures seller surplus from trade.", "Used in welfare and total-surplus questions.", "Producer Surplus", "Deadweight Loss", true, false, 108, 138},
};

static const GraphElementEntry k_ppc_elements[] = {
    {"frontier", "Production Possibilities Curve", "curve", "Bow-shaped production frontier", "Shows max output combinations with current resources and technology.", "Classify efficient, inefficient, and unattainable points.", "Production Possibilities Curve, Scarcity", "", false, true, 184, 74},
    {"opportunity-cost", "Opportunity Cost", "movement", "Slope and movement along the PPC frontier", "Shows how much of one good must be given up to get more of the other.", "Interpret trade-offs and comparative advantage logic.", "Opportunity Cost, Trade-Off", "", false, true, 146, 96},
    {"efficient", "Efficient", "point", "Point on the PPC frontier", "Uses all available resources productively.", "Identify productive efficiency.", "Efficient, Production Possibilities Curve", "", false, false, 164, 130},
    {"inefficient", "Inefficient", "point", "Point inside the PPC", "Shows unemployment or underused resources.", "Spot idle resources.", "Inefficient, Scarcity", "", false, false, 112, 158},
    {"unattainable", "Unattainable", "point", "Point outside the PPC", "Cannot be reached with current resources and technology.", "Distinguish current limits from growth.", "Unattainable, Productivity", "", false, true, 220, 60},
    {"growth-shift", "PPC Growth Shift", "curve", "Dashed frontier outside the original PPC", "Shows greater productive capacity from more resources, better technology, or productivity.", "Explain outward PPC shifts.", "Productivity, Production Possibilities Curve", "", false, true, 198, 46},
};

static const GraphElementEntry k_trade_table_elements[] = {
    {"absolute-advantage", "Absolute Advantage", "table comparison", "Higher total-output cells in the production table", "Shows who can produce more with the same resources.", "Separate raw output from opportunity-cost logic.", "Absolute Advantage", "", false, false, 136, 72},
    {"comparative-advantage", "Comparative Advantage", "table comparison", "Lower opportunity-cost row in the trade table", "Shows who should specialize in each good.", "Use lower opportunity cost to assign specialization.", "Comparative Advantage, Opportunity Cost", "", false, false, 136, 110},
    {"specialization", "Specialization", "decision outcome", "Highlighted output choice for each producer", "Shows each producer focusing on the good with lower opportunity cost.", "Connect comparative advantage to production decisions.", "Specialization, Comparative Advantage", "", false, false, 136, 144},
    {"gains-from-trade", "Gains from Trade", "outcome note", "Trade-outcome line showing both sides can benefit", "Shows how trade can improve consumption possibilities for both sides.", "Check whether terms of trade fall between opportunity costs.", "Gains from Trade, Terms of Trade", "", false, false, 136, 172},
};

static const GraphElementEntry k_tax_elements[] = {
    {"taxed-supply", "Taxed Supply", "curve", "Supply curve shifted up by the tax", "Creates a wedge between buyer and seller prices.", "Show tax incidence and output reduction.", "Tax Incidence, Deadweight Loss", "Tax Revenue, Deadweight Loss", false, true, 214, 64},
    {"buyer-price", "Buyer Price", "point", "Higher post-tax price buyers pay", "Shows the buyer side of the tax burden.", "Compare with seller price to measure the wedge.", "Tax Incidence", "Tax Revenue", false, false, 174, 100},
    {"seller-price", "Seller Price", "point", "Lower post-tax price sellers receive", "Shows the seller side of the tax burden.", "Compare with buyer price to measure the wedge.", "Tax Incidence", "Tax Revenue", false, false, 174, 144},
    {"tax-revenue", "Tax Revenue", "region", "Rectangle between Pb and Ps over post-tax quantity", "Government revenue from the per-unit tax.", "Area questions on tax graphs.", "Tax Incidence", "Tax Revenue", true, false, 138, 122},
    {"deadweight-loss", "Deadweight Loss", "region", "Triangle to the right of post-tax quantity", "Lost trades caused by the tax.", "Efficiency loss from market intervention.", "Deadweight Loss", "Deadweight Loss", true, false, 194, 132},
};

static const GraphElementEntry k_subsidy_elements[] = {
    {"subsidized-supply", "Subsidized Supply", "curve", "Supply curve shifted down by the subsidy", "Creates a wedge that increases quantity.", "Show overproduction and government spending.", "Subsidy", "Tax Revenue, Deadweight Loss", false, true, 214, 96},
    {"buyer-price", "Buyer Price", "point", "Lower price paid by buyers", "Shows the consumer side of the subsidy.", "Compare with seller price to see the wedge.", "Subsidy", "", false, false, 184, 128},
    {"seller-price", "Seller Price", "point", "Higher price received by sellers", "Shows the producer side of the subsidy.", "Compare with buyer price to see the wedge.", "Subsidy", "", false, false, 184, 92},
    {"government-spending", "Government Spending", "region", "Rectangle of subsidy per unit times quantity", "Total cost of the subsidy.", "Area questions on subsidy graphs.", "Subsidy", "Tax Revenue", true, false, 146, 108},
    {"deadweight-loss", "Deadweight Loss", "region", "Triangle from overproduction beyond the efficient output", "Lost welfare from producing too much.", "Efficiency loss from subsidy distortion.", "Deadweight Loss", "Deadweight Loss", true, false, 218, 116},
};

static const GraphElementEntry k_ceiling_elements[] = {
    {"ceiling", "Price Ceiling", "line", "Horizontal line below equilibrium", "A binding ceiling creates shortage.", "Recognize a legal maximum price.", "Price Ceiling, Shortage", "", false, false, 190, 130},
    {"quantity-supplied", "Quantity Supplied", "point", "Left quantity at the ceiling", "Actual exchange is limited by the short side of the market.", "Identify traded quantity under a ceiling.", "Shortage", "", false, false, 128, 162},
    {"quantity-demanded", "Quantity Demanded", "point", "Right quantity at the ceiling", "Shows how much buyers want at the ceiling price.", "Compare with Qs to find shortage.", "Shortage", "", false, false, 188, 162},
    {"shortage", "Shortage", "region", "Horizontal gap between Qd and Qs", "Quantity demanded exceeds quantity supplied.", "Main AP effect of a binding ceiling.", "Shortage", "", true, false, 158, 152},
};

static const GraphElementEntry k_floor_elements[] = {
    {"floor", "Price Floor", "line", "Horizontal line above equilibrium", "A binding floor creates surplus.", "Recognize a legal minimum price.", "Price Floor, Surplus", "", false, false, 188, 90},
    {"quantity-demanded", "Quantity Demanded", "point", "Left quantity at the floor", "Shows how much buyers want at the floor price.", "Use with Qs to find surplus.", "Surplus", "", false, false, 118, 162},
    {"quantity-supplied", "Quantity Supplied", "point", "Right quantity at the floor", "Shows how much sellers want to sell at the floor.", "Use with Qd to find surplus.", "Surplus", "", false, false, 198, 162},
    {"surplus", "Surplus", "region", "Horizontal gap between Qs and Qd", "Quantity supplied exceeds quantity demanded.", "Main AP effect of a binding floor.", "Surplus", "", true, false, 156, 150},
};

static const GraphElementEntry k_cost_elements[] = {
    {"mc", "Marginal Cost", "curve", "Upward-sloping curve cutting AVC and ATC at their minimums", "Shows the cost of the next unit.", "Profit-max rule uses MR = MC.", "Marginal Cost", "Marginal Cost, Economic Profit", false, false, 210, 70},
    {"avc", "Average Variable Cost", "curve", "U-shaped curve below ATC", "Variable cost per unit.", "Shutdown uses price vs AVC.", "Average Variable Cost, Shutdown Rule", "Average Cost Family", false, false, 204, 96},
    {"atc", "Average Total Cost", "curve", "U-shaped curve above AVC", "Total cost per unit.", "Compare price with ATC for profit or loss.", "Average Total Cost, Normal Profit", "Average Cost Family, Economic Profit", false, false, 204, 122},
    {"afc", "Average Fixed Cost", "curve", "Always falling curve", "Fixed cost spread over output.", "Explain why ATC falls early on.", "Average Fixed Cost", "Average Cost Family", false, false, 204, 148},
};

static const GraphElementEntry k_pc_firm_elements[] = {
    {"price-mr", "Price = MR", "line", "Horizontal market-price line", "Competitive firms are price takers.", "Use as demand, AR, and MR for the firm.", "Normal Profit, Shutdown Rule", "Marginal Revenue Basics", false, true, 190, 114},
    {"mc", "Marginal Cost", "curve", "Upward-sloping curve", "MC determines profit-max quantity with MR.", "Find Q where MR = MC.", "Marginal Cost", "Marginal Cost", false, false, 212, 76},
    {"atc", "Average Total Cost", "curve", "U-shaped cost curve", "Use to identify profit, loss, or break-even.", "Compare price with ATC at Q*", "Average Total Cost, Normal Profit", "Economic Profit", false, false, 210, 122},
    {"profit-quantity", "Profit-Max Quantity", "point", "Where MR meets MC", "Best output rule for the firm.", "Always solve Q first.", "Normal Profit", "Marginal Revenue Basics", false, false, 168, 114},
    {"profit", "Profit Rectangle", "region", "Between price and ATC at Q*", "Positive economic profit when price exceeds ATC.", "Area of profit or loss.", "Economic Profit", "Economic Profit", true, false, 146, 100},
};

static const GraphElementEntry k_pc_market_elements[] = {
    {"demand", "Market Demand", "curve", "Downward-sloping market demand", "Shows all buyers in the market.", "Sets the equilibrium market price.", "Market Equilibrium", "Total Revenue", false, true, 214, 152},
    {"supply", "Market Supply", "curve", "Upward-sloping market supply", "Shows all firms in the market.", "Intersects demand at market equilibrium.", "Market Equilibrium", "", false, true, 214, 68},
    {"equilibrium", "Market Equilibrium", "point", "Intersection of market supply and demand", "Sets the price each competitive firm takes as given.", "Link market graph to firm graph.", "Market Equilibrium", "", false, true, 156, 114},
};

static const GraphElementEntry k_monopoly_elements[] = {
    {"demand", "Demand Curve", "curve", "Downward-sloping market demand", "Shows the price buyers pay at each quantity.", "After MR = MC gives Q, go up to demand for price.", "Monopoly, Price Maker, Price Discrimination", "Total Revenue, Imperfect Competition Profit Rule", false, false, 212, 90},
    {"mr", "Marginal Revenue", "curve", "Curve below demand", "Extra revenue from one more unit.", "Use with MC to find monopoly quantity.", "Marginal Revenue", "Marginal Revenue Basics, Imperfect Competition Profit Rule", false, false, 212, 144},
    {"mc", "Marginal Cost", "curve", "Upward-sloping curve", "Cost of the next unit.", "Profit-max quantity occurs where MR = MC.", "Marginal Cost", "Marginal Cost", false, false, 204, 128},
    {"profit-max-q", "Profit-Max Quantity", "point", "MR = MC quantity", "Output restriction by monopoly.", "First step of monopoly analysis.", "Monopoly, Profit-Maximizing Quantity", "Marginal Revenue Basics, Imperfect Competition Profit Rule", false, false, 166, 130},
    {"deadweight-loss", "Deadweight Loss", "region", "Triangle between monopoly output and efficient output", "Represents mutually beneficial trades not made.", "Allocative inefficiency under monopoly.", "Deadweight Loss, Allocative Inefficiency", "Deadweight Loss, Efficiency in Imperfect Competition", true, true, 196, 106},
};

static const GraphElementEntry k_monopolistic_elements[] = {
    {"demand", "Demand Curve", "curve", "Downward-sloping demand", "Firm has some market power from differentiation.", "Read price from demand.", "Monopolistic Competition, Price Maker", "Economic Profit, Imperfect Competition Profit Rule", false, false, 210, 98},
    {"mr", "Marginal Revenue", "curve", "Curve below demand", "Extra revenue from one more unit.", "Use with MC to find output.", "Marginal Revenue", "Marginal Revenue Basics, Imperfect Competition Profit Rule", false, false, 208, 140},
    {"atc", "Average Total Cost", "curve", "U-shaped ATC tangent in long run", "In long run, price equals ATC at chosen output.", "Zero economic profit with excess capacity.", "Normal Profit", "Economic Profit", false, false, 204, 118},
    {"excess-capacity", "Excess Capacity", "region", "Gap between efficient output and chosen output", "Firm does not produce at minimum ATC.", "Common AP comparison with perfect competition.", "Monopolistic Competition, Excess Capacity, Productive Inefficiency", "Efficiency in Imperfect Competition", true, true, 190, 152},
};

static const GraphElementEntry k_kinked_demand_elements[] = {
    {"demand", "Kinked Demand Curve", "curve", "Demand curve with a visible kink at the current price-output point", "Shows that rivals may match price cuts but ignore price increases.", "Used to explain price rigidity in oligopoly.", "Oligopoly", "Marginal Revenue Basics", false, true, 206, 84},
    {"mr-gap", "MR Gap", "curve", "Broken marginal-revenue segment below the kink", "Shows the discontinuity that can keep price and output stable.", "If MC changes inside this gap, the firm may not change price.", "Oligopoly, Kinked Demand", "Marginal Revenue Basics", false, true, 170, 128},
    {"price-rigidity", "Price Rigidity", "point", "The kink point where the current price tends to stick", "Represents sticky pricing under oligopoly.", "Shows why oligopoly prices may remain unchanged after small cost changes.", "Oligopoly", "", false, true, 148, 98},
};

static const GraphElementEntry k_game_matrix_elements[] = {
    {"collusive-outcome", "Collusive Outcome", "matrix cell", "Top-left payoff cell with higher joint profit", "Shows the cooperative or high-price outcome.", "Useful for comparing joint profit with the Nash equilibrium.", "Collusion, Cartel", "Total Revenue", true, false, 104, 82},
    {"dominant-strategy", "Dominant Strategy", "matrix strategy", "The row or column choice that is best regardless of the rival's move", "Best response in every case for one player.", "Used to solve AP payoff-matrix questions quickly.", "Dominant Strategy, Game Theory", "", false, false, 184, 58},
    {"nash-equilibrium", "Nash Equilibrium", "matrix cell", "Bottom-right payoff cell where neither player wants to move alone", "Stable strategic outcome given rival choices.", "Key answer for oligopoly payoff-matrix questions.", "Nash Equilibrium, Game Theory", "", true, false, 188, 136},
    {"prisoners-dilemma", "Prisoner's Dilemma", "matrix logic", "Comparison between the collusive cell and the Nash-equilibrium cell", "Shows why rational individual strategies can produce a worse joint result.", "Explains why collusion is unstable.", "Prisoner's Dilemma, Collusion, Oligopoly", "", false, false, 146, 110},
};

static const GraphElementEntry k_monopsony_elements[] = {
    {"labor-supply", "Labor Supply", "curve", "Upward-sloping labor supply", "Shows the wage needed to attract more workers.", "Read the monopsony wage from labor supply after the hiring quantity is chosen.", "Labor Supply, Monopsony", "", false, false, 212, 70},
    {"mfc", "Marginal Resource Cost", "curve", "Curve above labor supply", "Extra cost of one more worker when hiring more labor raises wages.", "Monopsony hires where MRP = MRC.", "Marginal Resource Cost, Marginal Factor Cost, Monopsony", "Hiring Rule for Labor", false, false, 210, 38},
    {"mrp", "Marginal Revenue Product", "curve", "Downward-sloping input demand", "Benefit of one more worker to the firm.", "Demand for labor in the factor market.", "Marginal Revenue Product, Labor Demand, Demand for Labor", "Marginal Revenue Product", false, false, 212, 152},
    {"hire-quantity", "Profit-Maximizing Quantity of Labor", "point", "Where MRP meets MRC", "Monopsony employment choice.", "Read wage from supply at this quantity and compare with the competitive outcome.", "Profit-Maximizing Quantity of Labor, Hiring Rule, Monopsony", "Hiring Rule for Labor", false, false, 174, 114},
    {"wage", "Monopsony Wage", "point", "Supply-curve wage at the chosen quantity", "Wage paid by the monopsonist.", "Compare it with the competitive equilibrium wage to show underpayment.", "Wage Determination, Equilibrium Wage, Monopsony", "", false, false, 174, 138},
};

static const GraphElementEntry k_production_elements[] = {
    {"tp", "Total Product", "curve", "Rising curve that flattens", "Total output from labor.", "Show increasing then diminishing returns.", "Production Function", "", false, false, 206, 62},
    {"mp", "Marginal Product", "curve", "Hump-shaped curve", "Extra output from one more worker.", "When MP falls, MC rises.", "Diminishing Marginal Returns", "Marginal Cost", false, false, 176, 116},
    {"diminishing-returns", "Diminishing Returns", "point", "Where MP starts falling", "Additional workers add less output than before.", "Key reason short-run MC rises.", "Diminishing Marginal Returns", "Marginal Cost", false, false, 142, 92},
};

static const GraphElementEntry k_labor_market_elements[] = {
    {"mrp", "Labor Demand", "curve", "Downward-sloping labor demand", "Shows the value of workers to firms and reflects derived demand.", "Demand shifts with productivity and output price.", "Labor Demand, Demand for Labor, Derived Demand, Marginal Revenue Product", "Marginal Revenue Product", false, true, 212, 152},
    {"labor-supply", "Labor Supply", "curve", "Upward-sloping labor supply", "Shows the wage needed to attract labor.", "Supply shifts with worker preferences, training, demographics, and labor-force size.", "Labor Supply, Resource Supply Shifters", "", false, true, 212, 70},
    {"labor-equilibrium", "Labor Market Equilibrium", "point", "Intersection of labor demand and labor supply", "The competitive outcome that sets wage and employment.", "Use it to read both equilibrium wage and equilibrium quantity of labor.", "Labor Market Equilibrium, Competitive Labor Market", "Hiring Rule for Labor", false, true, 166, 114},
    {"equilibrium-wage", "Equilibrium Wage", "point", "Wage read on the vertical axis from the market intersection", "Market-clearing wage rate.", "Compare wages before and after a shift or against monopsony.", "Equilibrium Wage, Wage Determination", "", false, true, 166, 114},
    {"labor-quantity", "Equilibrium Quantity of Labor", "point", "Labor quantity read on the horizontal axis from the market intersection", "Market-clearing employment.", "Read labor hired from the equilibrium point.", "Equilibrium Quantity of Labor, Competitive Labor Market", "", false, true, 166, 114},
};

static const GraphElementEntry k_hiring_rule_elements[] = {
    {"mrp", "Marginal Revenue Product", "curve", "Downward-sloping MRP curve", "Shows the added revenue from one more worker.", "If MRP is above MRC, the firm should hire more labor.", "Marginal Revenue Product, Labor Demand, Demand for Labor", "Marginal Revenue Product", false, true, 212, 132},
    {"wage-mrc", "Marginal Resource Cost", "line", "Horizontal wage = MRC line for a wage-taking firm", "Shows the cost of one more worker in a competitive labor market.", "A higher wage shifts this line up and reduces hiring.", "Marginal Resource Cost, Hiring Rule", "Hiring Rule for Labor", false, true, 210, 74},
    {"hire-quantity", "Profit-Maximizing Quantity of Labor", "point", "Where MRP meets wage = MRC", "The number of workers the firm should hire.", "This is the hiring rule outcome for a competitive labor-market firm.", "Profit-Maximizing Quantity of Labor, Hiring Rule", "Hiring Rule for Labor", false, true, 166, 104},
};

static const GraphElementEntry k_surplus_elements[] = {
    {"consumer-surplus", "Consumer Surplus", "region", "Triangle above price and below demand", "Measures buyers' gains from trade.", "Used in welfare and intervention analysis.", "Consumer Surplus", "Deadweight Loss", true, false, 110, 86},
    {"producer-surplus", "Producer Surplus", "region", "Triangle below price and above supply", "Measures sellers' gains from trade.", "Used in welfare and intervention analysis.", "Producer Surplus", "Deadweight Loss", true, false, 112, 138},
    {"equilibrium", "Equilibrium", "point", "Intersection with market price line", "Sets the trade price and quantity.", "Base of total-surplus analysis.", "Market Equilibrium", "", false, false, 156, 114},
    {"total-surplus", "Total Surplus", "region", "Consumer surplus plus producer surplus", "Overall gains from trade.", "Benchmark for deadweight-loss questions.", "Consumer Surplus, Producer Surplus", "Deadweight Loss", true, false, 132, 112},
};

static const GraphElementEntry k_negative_externality_elements[] = {
    {"mpc", "MPC", "curve", "Lower private-cost curve", "Private market cost without the external cost.", "Compare with MSC to show overproduction.", "MPC, Negative Externality, Externality", "", false, false, 214, 92},
    {"msc", "MSC", "curve", "Higher social-cost curve", "Includes private cost plus external cost.", "Find socially optimal quantity with MSB.", "MSC, Social Cost, Negative Externality, Externality", "", false, false, 214, 56},
    {"msb", "MSB", "curve", "Benefit curve", "Benefit side used to compare the private outcome with the social optimum.", "Compare market output with socially optimal output.", "MSB, Socially Efficient Outcome, Externality", "", false, false, 214, 152},
    {"market-quantity", "Market Quantity", "point", "Where MPC meets MSB", "Private-market outcome before correction.", "Shows overproduction relative to the social optimum.", "Negative Externality, Externality", "", false, false, 156, 108},
    {"socially-optimal", "Socially Optimal Quantity", "point", "Where MSC meets MSB", "Efficient output with full social costs.", "Target for corrective taxes or regulation.", "Socially Optimal Quantity, Socially Efficient Outcome", "Deadweight Loss", false, false, 134, 92},
    {"external-cost", "Social Cost", "gap", "Vertical gap between MSC and MPC", "Shows the external cost not counted by the private market.", "Explain why MSC is above MPC.", "Social Cost, Negative Externality, Externality", "", true, false, 182, 82},
    {"corrective-tax", "Corrective Tax", "policy", "Policy move from market quantity back toward social optimum", "Internalizes the external cost and reduces overproduction.", "Use for policy questions and regulation comparisons.", "Corrective Tax, Regulation, Government Intervention", "Deadweight Loss", false, true, 146, 64},
    {"deadweight-loss", "Deadweight Loss", "region", "Triangle between market and efficient quantities", "Lost welfare from overproduction.", "Policy correction question.", "Deadweight Loss, Socially Efficient Outcome", "Deadweight Loss", true, false, 144, 118},
};

static const GraphElementEntry k_positive_externality_elements[] = {
    {"mpb", "MPB", "curve", "Lower private-benefit curve", "Private willingness to buy or consume.", "Compare with MSB to show underproduction.", "MPB, Positive Externality, Externality", "", false, false, 214, 106},
    {"msb", "MSB", "curve", "Higher social-benefit curve", "Includes private benefit plus external benefit.", "Find socially optimal quantity with MSC.", "MSB, Social Benefit, Positive Externality, Externality", "", false, false, 214, 68},
    {"msc", "MSC", "curve", "Cost curve", "Social cost side of the market.", "Compare market output with the social optimum.", "MSC, Socially Efficient Outcome, Externality", "", false, false, 214, 152},
    {"market-quantity", "Market Quantity", "point", "Where MPB meets MSC", "Private-market outcome before correction.", "Shows underproduction relative to the social optimum.", "Positive Externality, Externality", "", false, false, 134, 122},
    {"socially-optimal", "Socially Optimal Quantity", "point", "Where MSB meets MSC", "Efficient output including external benefits.", "Target for corrective subsidies or public support.", "Socially Optimal Quantity, Socially Efficient Outcome", "Deadweight Loss", false, false, 160, 102},
    {"external-benefit", "Social Benefit", "gap", "Vertical gap between MSB and MPB", "Shows the external benefit not counted by the private market.", "Explain why MSB is above MPB.", "Social Benefit, Positive Externality, Externality", "", true, false, 180, 86},
    {"subsidy", "Subsidy", "policy", "Policy move from market quantity toward social optimum", "Raises output by internalizing the external benefit.", "Use for policy questions about correcting underproduction.", "Subsidy, Government Intervention, Positive Externality", "Deadweight Loss", false, true, 152, 76},
    {"deadweight-loss", "Deadweight Loss", "region", "Triangle between market and efficient quantities", "Lost welfare from underproduction.", "Policy correction question.", "Deadweight Loss, Socially Efficient Outcome", "Deadweight Loss", true, false, 150, 118},
};

static const GraphElementEntry k_public_goods_common_resources_elements[] = {
    {"public-good", "Public Good", "classification", "Left panel: non-rival and non-excludable", "A good many people can consume at once without easy exclusion.", "Classify the good and connect it to free-rider logic.", "Public Good", "", false, false, 84, 64},
    {"free-rider", "Free Rider", "behavior", "Left panel example of a nonpayer receiving benefits", "A person who benefits without paying.", "Explain why private sellers struggle to collect enough revenue.", "Free Rider, Free Rider Problem, Public Good", "", false, false, 96, 112},
    {"free-rider-problem", "Free Rider Problem", "market failure", "Left panel underprovision note", "Private demand understates the full social benefit of a public good.", "Use for government provision questions.", "Free Rider Problem, Public Good", "Deadweight Loss", false, false, 104, 144},
    {"common-resource", "Common Resource", "classification", "Right panel: rival and non-excludable", "A good each user can access, but each use leaves less for others.", "Classify the good and connect it to overuse logic.", "Common Resource", "", false, false, 248, 64},
    {"tragedy-of-the-commons", "Tragedy of the Commons", "overuse", "Right panel overuse note", "Open access pushes use beyond the socially efficient amount.", "Use for regulation, permit, or property-right questions.", "Tragedy of the Commons, Regulation, Common Resource", "Deadweight Loss", false, true, 256, 138},
};

static const GraphElementEntry k_lorenz_elements[] = {
    {"equality", "Line of Equality", "line", "45-degree benchmark line", "Perfect equality reference.", "Compare with Lorenz curve to judge inequality.", "Lorenz Curve, Gini Coefficient", "", false, false, 202, 52},
    {"lorenz", "Lorenz Curve", "curve", "Bow-shaped curve below equality", "Actual cumulative income distribution.", "Farther from equality means more inequality.", "Lorenz Curve", "", false, false, 194, 112},
    {"gini-area", "Gini Area", "region", "Area between Lorenz curve and equality line", "Visual basis for the Gini coefficient.", "Compare inequality across societies.", "Gini Coefficient", "", true, false, 146, 86},
};

typedef struct {
    const char *graph_id;
    const GraphElementEntry *elements;
    int count;
} GraphElementMap;

typedef struct {
    const char *graph_id;
    GraphCapabilities capabilities;
} GraphCapabilitiesMap;

static const GraphElementMap k_graph_maps[] = {
    {"supply-demand", k_supply_demand_elements, sizeof k_supply_demand_elements / sizeof k_supply_demand_elements[0]},
    {"ppc", k_ppc_elements, sizeof k_ppc_elements / sizeof k_ppc_elements[0]},
    {"trade-table", k_trade_table_elements, sizeof k_trade_table_elements / sizeof k_trade_table_elements[0]},
    {"tax", k_tax_elements, sizeof k_tax_elements / sizeof k_tax_elements[0]},
    {"subsidy", k_subsidy_elements, sizeof k_subsidy_elements / sizeof k_subsidy_elements[0]},
    {"price-ceiling", k_ceiling_elements, sizeof k_ceiling_elements / sizeof k_ceiling_elements[0]},
    {"price-floor", k_floor_elements, sizeof k_floor_elements / sizeof k_floor_elements[0]},
    {"cost-curves", k_cost_elements, sizeof k_cost_elements / sizeof k_cost_elements[0]},
    {"perfect-competition", k_pc_firm_elements, sizeof k_pc_firm_elements / sizeof k_pc_firm_elements[0]},
    {"perfect-competition-market", k_pc_market_elements, sizeof k_pc_market_elements / sizeof k_pc_market_elements[0]},
    {"monopoly", k_monopoly_elements, sizeof k_monopoly_elements / sizeof k_monopoly_elements[0]},
    {"monopolistic-competition", k_monopolistic_elements, sizeof k_monopolistic_elements / sizeof k_monopolistic_elements[0]},
    {"kinked-demand", k_kinked_demand_elements, sizeof k_kinked_demand_elements / sizeof k_kinked_demand_elements[0]},
    {"game-theory-matrix", k_game_matrix_elements, sizeof k_game_matrix_elements / sizeof k_game_matrix_elements[0]},
    {"monopsony", k_monopsony_elements, sizeof k_monopsony_elements / sizeof k_monopsony_elements[0]},
    {"production-function", k_production_elements, sizeof k_production_elements / sizeof k_production_elements[0]},
    {"labor-market", k_labor_market_elements, sizeof k_labor_market_elements / sizeof k_labor_market_elements[0]},
    {"hiring-rule", k_hiring_rule_elements, sizeof k_hiring_rule_elements / sizeof k_hiring_rule_elements[0]},
    {"surplus-welfare", k_surplus_elements, sizeof k_surplus_elements / sizeof k_surplus_elements[0]},
    {"negative-externality", k_negative_externality_elements, sizeof k_negative_externality_elements / sizeof k_negative_externality_elements[0]},
    {"positive-externality", k_positive_externality_elements, sizeof k_positive_externality_elements / sizeof k_positive_externality_elements[0]},
    {"public-goods-common-resources", k_public_goods_common_resources_elements, sizeof k_public_goods_common_resources_elements / sizeof k_public_goods_common_resources_elements[0]},
    {"lorenz", k_lorenz_elements, sizeof k_lorenz_elements / sizeof k_lorenz_elements[0]},
};

static const GraphCapabilitiesMap k_graph_capabilities[] = {
    {
        "supply-demand",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_REGIONS},
            {"Labels", "Points", "Areas"},
        },
    },
    {
        "surplus-welfare",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "tax",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "subsidy",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "price-ceiling",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "price-floor",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "cost-curves",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "perfect-competition",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "perfect-competition-market",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_REGIONS},
            {"Labels", "Points", "Areas"},
        },
    },
    {
        "monopoly",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "monopolistic-competition",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "kinked-demand",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "game-theory-matrix",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "monopsony",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "production-function",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "labor-market",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "hiring-rule",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "negative-externality",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "positive-externality",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
    {
        "public-goods-common-resources",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "ppc",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_SHIFT | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_POINTS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_POINTS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Points", "Concepts"},
        },
    },
    {
        "trade-table",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Highlights", "Concepts"},
        },
    },
    {
        "lorenz",
        {
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_CONCEPTS | GRAPH_FEATURE_INFO,
            GRAPH_FEATURE_LABELS | GRAPH_FEATURE_REGIONS | GRAPH_FEATURE_INFO,
            {GRAPH_FEATURE_LABELS, GRAPH_FEATURE_REGIONS, GRAPH_FEATURE_CONCEPTS},
            {"Labels", "Areas", "Concepts"},
        },
    },
};

static const GraphElementMap *find_graph_map(const GraphEntry *graph)
{
    int i = 0;
    if(!graph) return NULL;
    for(i = 0; i < (int)(sizeof k_graph_maps / sizeof k_graph_maps[0]); i++) {
        if(strcmp(k_graph_maps[i].graph_id, graph->id) == 0) return &k_graph_maps[i];
    }
    return NULL;
}

static const GraphCapabilitiesMap *find_capability_map(const GraphEntry *graph)
{
    int i = 0;
    if(!graph) return NULL;
    for(i = 0; i < (int)(sizeof k_graph_capabilities / sizeof k_graph_capabilities[0]); i++) {
        if(strcmp(k_graph_capabilities[i].graph_id, graph->id) == 0) return &k_graph_capabilities[i];
    }
    return NULL;
}

static bool csv_contains(const char *csv, const char *term)
{
    size_t term_len = 0;
    const char *cursor = csv;
    if(!csv || !term || !term[0]) return false;
    term_len = strlen(term);
    while(*cursor) {
        while(*cursor == ' ' || *cursor == ',') cursor++;
        if(strncmp(cursor, term, term_len) == 0) {
            char end = cursor[term_len];
            if(end == 0 || end == ',') return true;
        }
        while(*cursor && *cursor != ',') cursor++;
    }
    return false;
}

int graphs_get_element_count(const GraphEntry *graph)
{
    const GraphElementMap *map = find_graph_map(graph);
    return map ? map->count : 0;
}

const GraphElementEntry *graphs_get_element(const GraphEntry *graph, int index)
{
    const GraphElementMap *map = find_graph_map(graph);
    if(!map || index < 0 || index >= map->count) return NULL;
    return &map->elements[index];
}

int graphs_find_element_by_id(const GraphEntry *graph, const char *id)
{
    const GraphElementMap *map = find_graph_map(graph);
    int i = 0;
    if(!map || !id || !id[0]) return -1;
    for(i = 0; i < map->count; i++) {
        if(strcmp(map->elements[i].id, id) == 0) return i;
    }
    return -1;
}

int graphs_find_element_by_term(const GraphEntry *graph, const char *term)
{
    const GraphElementMap *map = find_graph_map(graph);
    int i = 0;
    if(!map || !term || !term[0]) return -1;
    for(i = 0; i < map->count; i++) {
        const GraphElementEntry *entry = &map->elements[i];
        if(strcmp(entry->name, term) == 0) return i;
        if(csv_contains(entry->related_terms, term)) return i;
    }
    return -1;
}

const GraphCapabilities *graphs_get_capabilities(const GraphEntry *graph)
{
    const GraphCapabilitiesMap *map = find_capability_map(graph);
    return map ? &map->capabilities : NULL;
}
