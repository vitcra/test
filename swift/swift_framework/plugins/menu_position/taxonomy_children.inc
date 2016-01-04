<?php
/**
 * @file
 * Provides the "Taxonomy" rule plugin for the Menu Position module.
 */

/**
 * Checks if a specific taxonomy term or any of its children is set in the node.
 *
 * @param $variables
 *   An array containing each of the variables saved in the database necessary
 *   to evaluate this condition of the rule.
 * @return boolean
 *   TRUE if condition applies successfully. Otherwise FALSE.
 */
function swift_framework_menu_position_condition_taxonomy_children($variables) {
  // We can only check for taxonomy terms on an "entity" page.
  if ($variables['context']['entity_type']) {

    // Grab the variables stored statically in the rule.
    $vid = $variables['vid'];
    $tid = $variables['tid'];

    if (empty($tid)) {
      return FALSE;
    }

    // Determine what kind of entity page this is.
    $entity_type = $variables['context']['entity_type'];
    $bundle_name = $variables['context']['bundle_name'];
    $entity = $variables['context'][$entity_type];

    // Build a list of each taxonomy reference fields.
    $taxonomy_fields = &drupal_static(__FUNCTION__, NULL);
    if (!isset($taxonomy_fields)) {
      $taxonomy_fields = array();
      $field_info = field_info_fields();
      foreach (array_keys(field_info_instances($entity_type, $bundle_name)) as $key) {
        if ($field_info[$key]['type'] == 'taxonomy_term_reference') {
          $taxonomy_fields[$key] = $field_info[$key]['translatable'];
        }
      }
    }

    foreach ($taxonomy_fields as $field => $translatable) {
      $language = $translatable ? $entity->language : LANGUAGE_NONE;
      if (!empty($entity->{$field}[$language])) {
        // First add child terms if necessary.
        foreach ($tid as $term_id) {
          $child_terms = taxonomy_get_children($term_id, $vid);
          $tid = array_unique(array_merge($tid, array_keys($child_terms)));
        }

        // Check for matching terms
        foreach ($entity->{$field}[$language] as $term) {
          if (in_array($term['tid'], $tid)) {
            return TRUE;
          }
        }
      }
    }
  }
  return FALSE;
}

/**
 * Adds form elements for the "taxonomy_children" plugin to the rule configuration form.
 *
 * @param $form
 *   A reference to the "add/edit rule" form array. New form elements should be
 *   added directly to this array.
 * @param $form_state
 *   A reference to the current form state.
 */
function swift_framework_menu_position_rule_taxonomy_children_form(&$form, &$form_state) {
  // If this is an existing rule, load the variables stored in the rule for this plugin.
  $variables = !empty($form_state['#menu-position-rule']['conditions']['taxonomy_children']) ? $form_state['#menu-position-rule']['conditions']['taxonomy_children'] : array();

  // Load the taxonomy terms.
  if (!empty($variables['tid'])) {
    $terms = array();
    foreach (taxonomy_term_load_multiple($variables['tid']) as $term) {
      $terms[] = $term->name;
    }
    $terms = implode(', ', $terms);
  }
  else {
    $terms = '';
  }

  $form['conditions']['taxonomy_children'] = array(
    '#type' => 'fieldset',
    '#title' => t('Taxonomy with children'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#attached' => array(
      'js' => array(drupal_get_path('module', 'swift_framework') . '/plugins/menu_position/taxonomy_children.js'),
    ),
  );
  $form['conditions']['taxonomy_children']['description'] = array(
    '#type' => 'item',
    '#description' => t('Apply this rule only on pages that display content having the given taxonomy term or any of its child terms.'),
  );
  $vocabularies = taxonomy_vocabulary_get_names();
  $vocabulary_options = array();
  foreach ($vocabularies as $vocabulary) {
    // $vocabulary['machine_name']
    $vocabulary_options[$vocabulary->vid] = $vocabulary->name;
  }
  $form['conditions']['taxonomy_children']['tc_vid'] = array(
    '#type' => 'select',
    '#title' => t('Vocabulary'),
    '#default_value' => !empty($variables['vid']) ? $variables['vid'] : '0',
    '#options' => array('0' => t('- None -')) + $vocabulary_options,
  );

//  $form['#attached']['js'][] = array(
//    'data' => array('menu_position_taxonomy_url' => url('menu-position/taxonomy/autocomplete')),
//    'type' => 'setting',
//  );

  $form['conditions']['taxonomy_children']['tc_term'] = array(
    '#type' => 'textfield',
    '#maxlength' => 1024,
    '#title' => t('Taxonomy term'),
    '#default_value' => $terms,
    '#autocomplete_path' => 'menu-position/taxonomy/autocomplete/' . (isset($form_state['input']['tc_vid']) ? $form_state['input']['tc_vid'] : $form['conditions']['taxonomy_children']['tc_vid']['#default_value']),
    '#element_validate' => array('swift_framework_menu_position_taxonomy_children_autocomplete_validate'),
    '#description' => t('Match at least one of these taxonomy terms.'),
  );

  // Add a submit handler.
  $form['#submit'][] = 'swift_framework_menu_position_rule_taxonomy_children_form_submit';
}

/**
 * Form element validate handler for taxonomy term autocomplete element.
 */
function swift_framework_menu_position_taxonomy_children_autocomplete_validate($element, &$form_state) {
  // Autocomplete widgets do not send their tids in the form, so we must detect
  // them here and process them independently.
  $value = array();

  if ($tags = $element['#value']) {
    // Translate term names into actual terms.
    $typed_terms = drupal_explode_tags($tags);
    foreach ($typed_terms as $typed_term) {
      // See if the term exists in the chosen vocabulary and return the tid;
      // otherwise, create a new 'autocreate' term for insert/update.
      if ($possibilities = taxonomy_term_load_multiple(array(), array('name' => trim($typed_term), 'vid' => array($form_state['values']['tc_vid'])))) {
        $term = array_pop($possibilities);
        $value[] = (array)$term;
      }
      else {
        form_set_error('term', t('%term is not a valid taxonomy term.', array('%term' => $typed_term)));
      }
    }
  }

  if (!empty($value)) {
    form_set_value($element, $value, $form_state);
  }
}

/**
 * Prepares the "taxonomy_children" variables to be stored in the rule.
 *
 * @param $form
 *   A reference to the "add/edit rule" form array.
 * @param $form_state
 *   A reference to the current form state, including submitted values.
 */
function swift_framework_menu_position_rule_taxonomy_children_form_submit(&$form, &$form_state) {
  if (!empty($form_state['values']['tc_vid']) && !empty($form_state['values']['tc_term'])) {
    $variables = array(
      'vid' => $form_state['values']['tc_vid'],
      'tid' => array(),
    );

    // Determine if a taxonomy term has been selected.
    foreach ($form_state['values']['tc_term'] as $term) {
      $variables['tid'][] = $term['tid'];
    }

    $form_state['values']['conditions']['taxonomy_children'] = $variables;
  }
}
