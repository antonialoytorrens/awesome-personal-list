/* awesome-personal-list - confirmation prompt for destructive forms, auto-submit for
 * the navbar language switcher and /sources filters, and the /sources
 * "select all on page" checkbox. External file so it runs under a strict
 * CSP (no inline script/handlers). */
var confirmDialog = document.getElementById("confirm-dialog");
var confirmMessage = document.getElementById("confirm-message");
var pendingForm = null;
var pendingSubmitter = null;
var bypassForm = null;

document.addEventListener("submit", function (ev) {
	var form = ev.target;
	var submitter = ev.submitter;
	var message;

	if (form === bypassForm) {
		bypassForm = null;
		return;
	}

	message = (submitter && submitter.getAttribute("data-confirm")) ||
		form.getAttribute("data-confirm");

	if (!message)
		return;

	ev.preventDefault();
	confirmMessage.textContent = message;
	pendingForm = form;
	pendingSubmitter = submitter;
	confirmDialog.showModal();
});

confirmDialog.addEventListener("close", function () {
	if (confirmDialog.returnValue === "confirm" && pendingForm) {
		bypassForm = pendingForm;
		pendingForm.requestSubmit(pendingSubmitter);
	}
	pendingForm = null;
	pendingSubmitter = null;
});

document.addEventListener("change", function (ev) {
	if (ev.target.classList.contains("lang-select-auto") ||
	    ev.target.classList.contains("filter-select-auto")) {
		ev.target.form.submit();
		return;
	}

	if (ev.target.classList.contains("select-all-check")) {
		var boxes = ev.target.form.querySelectorAll(".star-check");
		for (var i = 0; i < boxes.length; i++)
			boxes[i].checked = ev.target.checked;
	}
});
