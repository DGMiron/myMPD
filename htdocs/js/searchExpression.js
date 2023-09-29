"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module searchExpression_js */

/**
 * Parses search expressions and update the ui for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function handleSearchExpression(appid) {
    const searchStrEl = elGetById(appid + 'SearchStr');
    const searchCrumbEl = elGetById(appid + 'SearchCrumb');
    setFocus(searchStrEl);
    createSearchCrumbs(app.current.search, searchStrEl, searchCrumbEl);
    if (app.current.search === '') {
        searchStrEl.value = '';
    }
    selectTag(appid + 'SearchTags', appid + 'SearchTagsDesc', app.current.filter);
}

/**
 * Removes the search timer
 * @returns {void}
 */
function clearSearchTimer() {
    if (searchTimer !== null) {
        clearTimeout(searchTimer);
        searchTimer = null;
    }
}

/**
 * Initializes search elements for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function initSearchExpression(appid) {
    elGetById(appid + 'SearchTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            execSearchExpression(elGetById(appid + 'SearchStr').value);
        }
    }, false);

    elGetById(appid + 'SearchStr').addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        if (value !== '') {
            const op = getSelectValueId(appid + 'SearchMatch');
            const crumbEl = elGetById(appid + 'SearchCrumb');
            crumbEl.appendChild(createSearchCrumb(app.current.filter, op, value));
            elShow(crumbEl);
            this.value = '';
        }
        else {
            searchTimer = setTimeout(function() {
                execSearchExpression(value);
            }, searchTimerTimeout);
        }
    }, false);

    elGetById(appid + 'SearchStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            execSearchExpression(value);
        }, searchTimerTimeout);
    }, false);

    elGetById(appid + 'SearchCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            execSearchExpression('');
            elGetById(appid + 'SearchStr').updateBtn();
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            const searchStrEl = elGetById(appid + 'SearchStr');
            searchStrEl.value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag(appid + 'SearchTags', appid + 'SearchTagsDesc', getData(event.target, 'filter-tag'));
            elGetById(appid + 'SearchMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            execSearchExpression(searchStrEl.value);
            if (elGetById(appid + 'SearchCrumb').childElementCount === 0) {
                elHideId(appid + 'SearchCrumb');
            }
            searchStrEl.updateBtn();
        }
    }, false);

    elGetById(appid + 'SearchMatch').addEventListener('change', function() {
        execSearchExpression(elGetById(appid + 'SearchStr').value);
    }, false);
}

/**
 * Executes the search expression
 * @param {string} value search value
 * @returns {void}
 */
function execSearchExpression(value) {
    const expression = createSearchExpression(elGetById(app.id + 'SearchCrumb'), app.current.filter, getSelectValueId(app.id + 'SearchMatch'), value);
    appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}

/**
 * Creates the search breadcrumbs from a mpd search expression
 * @param {string} searchStr the search expression
 * @param {HTMLElement} searchEl search input element
 * @param {HTMLElement} crumbEl element to add the crumbs
 * @returns {void}
 */
function createSearchCrumbs(searchStr, searchEl, crumbEl) {
    elClear(crumbEl);
    const elements = searchStr.substring(1, app.current.search.length - 1).split(' AND ');
    //add all but last element to crumbs
    for (let i = 0, j = elements.length - 1; i < j; i++) {
        const fields = elements[i].match(/^\((\w+)\s+(\S+)\s+'(.*)'\)$/);
        if (fields !== null && fields.length === 4) {
            crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
        }
    }
    //check if we should add the last element to the crumbs
    if (searchEl.value === '' &&
        elements.length >= 1)
    {
        const fields = elements[elements.length - 1].match(/^\((\w+)\s+(\S+)\s+'(.*)'\)$/);
        if (fields !== null && fields.length === 4) {
            crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
        }
    }
    crumbEl.childElementCount > 0 ? elShow(crumbEl) : elHide(crumbEl);
}

/**
 * Creates a search crumb element
 * @param {string} filter the tag
 * @param {string} op search operator
 * @param {string} value filter value
 * @returns {HTMLElement} search crumb element
 */
function createSearchCrumb(filter, op, value) {
    const btn = elCreateNodes('button', {"class": ["btn", "btn-dark", "me-2"]}, [
        document.createTextNode(tn(filter) + ' ' + tn(op) + ' \'' + value + '\''),
        elCreateText('span', {"class": ["ml-2", "badge", "bg-secondary"]}, '×')
    ]);
    setData(btn, 'filter-tag', filter);
    setData(btn, 'filter-op', op);
    setData(btn, 'filter-value', value);
    return btn;
}

/**
 * Creates a MPD search expression
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function _createSearchExpression(tag, op, value) {
    if (op === 'starts_with' &&
        app.id !== 'BrowseDatabaseList' &&
        features.featStartsWith === false)
    {
        //mpd does not support starts_with, convert it to regex
        if (features.featPcre === true) {
            //regex is supported
            op = '=~';
            value = '^' + value;
        }
        else {
            //best option without starts_with and regex is contains
            op = 'contains';
        }
    }
    return '(' + tag + ' ' + op + ' ' +
        (op === '>=' ? value : '\'' + escapeMPD(value) + '\'') +
        ')';
}

/**
 * Creates the MPD search expression from crumbs and parameters
 * @param {HTMLElement} crumbsEl crumbs container element
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function createSearchExpression(crumbsEl, tag, op, value) {
    let expression = '(';
    const crumbs = crumbsEl.children;
    for (let i = 0, j = crumbs.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        expression += _createSearchExpression(
            getData(crumbs[i], 'filter-tag'),
            getData(crumbs[i], 'filter-op'),
            getData(crumbs[i], 'filter-value')
        );
    }
    if (value !== '') {
        if (expression.length > 1) {
            expression += ' AND ';
        }
        expression += _createSearchExpression(tag, op, value);
    }
    expression += ')';
    if (expression.length <= 2) {
        expression = '';
    }
    return expression;
}