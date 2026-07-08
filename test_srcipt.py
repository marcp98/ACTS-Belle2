import acts
# direkt nach Detray-Symbolen suchen:
print([x for x in dir(acts) if 'etray' in x.lower() or 'raccc' in x.lower()])

# auch in Untermodulen schauen:
import acts.examples
print([x for x in dir(acts.examples) if 'etray' in x.lower()])