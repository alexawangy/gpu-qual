#include "gpu_qual/verdict.hpp"
#include <gpu_qual/io_json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json gpu_qual::to_json(const Result &res) {
   json jres = {
     {"tool_version", res.tool_version},
     {"schema_version", res.schema_version},
     {"mode", gpu_qual::to_string(res.mode)},
     {"exit_code", static_cast<int>(res.exit_code)},
     {"verdict", gpu_qual::to_string(res.verdict)}
   };

   jres["reasons"] = json::array();

   for (const Reason &reason : res.reasons) {
     json jr = {
       {"code", gpu_qual::to_string(reason.code)},
       {"class", gpu_qual::to_string(reason.cls)}
     };

     if (!reason.field.empty()) jr["field"] = reason.field;
     if (!reason.expected.empty()) jr["expected"] = reason.expected;
     if (!reason.observed.empty()) jr["observed"] = reason.observed;

     jres["reasons"].push_back(jr);
   }

   return jres;
}
