covered_lines_current = set()
covered_lines_overall = set()
with open("instrumented_Problem10.c.gcov", "r") as gcov_file:
    for line in gcov_file:
        parts = line.split(":", 2) # Split into coverage, line number, code
        if len(parts) == 3:
            coverage_marker = parts[0].strip()
            try:
                execution_count = int(coverage_marker) # Check if it's a number
                line_number = int(parts[1].strip())
                covered_lines_current.add(line_number)
            except ValueError: # Not a number, could be '-', '#####', etc.
                pass

newly_covered_lines = covered_lines_current - covered_lines_overall
fitness_score = len(newly_covered_lines)

covered_lines_overall.update(covered_lines_current)

print(fitness_score)